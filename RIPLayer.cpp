#include "StdAfx.h"
#include "RIPLayer.h"
#include "RouterDlg.h"
CRIPLayer::CRIPLayer(char* pName) : CBaseLayer(pName)
{
	ResetHeader();
}

CRIPLayer::~CRIPLayer() { }

BOOL CRIPLayer::Send(unsigned char* ppayload, int nlength, int dev_num)
{
	unsigned char broadcast[4];
	memset(broadcast, 0xff, 4);
	unsigned char macbroadcast[6];
	memset(macbroadcast, 0xff, 6);

	CRouterDlg * routerDlg =  ((CRouterDlg *)GetUpperLayer(0));
	int messageLength;

	if (nlength == 1) {
		Rip_header.Rip_command = 0x01;
		CreateRequestMessage();

		// Request �� ��쿡�� �Ѱ���
		messageLength = 20;
	}

	if (nlength == 2) {
		Rip_header.Rip_command = 0x02;
		CreateResponseMessageTable();

		// Response�� ��쿡�� Routing table�� Entry ������ŭ (4Byte * 5��)
		messageLength = CRouterDlg::route_table.GetSize() * 20;
	}

	routerDlg->m_EthernetLayer->SetDestinAddress(macbroadcast, dev_num);
	routerDlg->m_IPLayer->SetDstIP(broadcast, dev_num);
	routerDlg->m_UDPLayer->SetSrcPort(0x0802); // 520(UDP)
	BOOL bSuccess = mp_UnderLayer->Send((unsigned char*)&Rip_header, RIP_HEADER_SIZE + messageLength, dev_num);
	return bSuccess;
}

BOOL CRIPLayer::Receive(unsigned char* ppayload, int dev_num)
{
	CRouterDlg::RoutingTable entry;
	CRouterDlg* routerDlg = ((CRouterDlg *) GetUpperLayer(0));
	PRipHeader pFrame = (PRipHeader) ppayload;

	// ���� Packet���� RIP Message�� �Ǹ� Entry�� ����(UDP ��ü ���̿��� UDP header(8), RIP �� ����(4) �� ����)
	unsigned short length = routerDlg->m_UDPLayer->GetLength(dev_num) - 12;

	if (pFrame->Rip_command == 0x01) { // command : Request�� ���� ���, command�� Response�� �����Ͽ� �ٽ� ����
		Send(0, 2, dev_num);
	}

	if (pFrame->Rip_command == 0x02) { // command : Response�� ���� ���, Routing table ������Ʈ

		// ���� Packet���� RIP Message�� �Ǹ� Entry�� ����(Entry �� ���̰� 20)
		int numOfEntries = length / 20;

		for (int index = 0; index < numOfEntries; index++) {
			CRouterDlg::RoutingTable entry;
			unsigned int metric = htonl(pFrame->Rip_table[index].Rip_metric);
			int selectIndex = ContainsRouteTableEntry(pFrame->Rip_table[index].Rip_ipAddress);

			if (selectIndex != -1) {	// �ش� IP�� �����Ѵٸ� ���Ͽ� Update
				entry = CRouterDlg::route_table.GetAt(CRouterDlg::route_table.FindIndex(selectIndex));

				/*
				 * routerDlg->m_IPLayer->GetSrcIP(2) : �� �κ��� next-hop�� ��Ÿ���� ���ΰ�? Ȯ���� �� !!!
				 */
				if (!memcmp((unsigned char*) routerDlg->m_IPLayer->GetSrcIP(2), pFrame->Rip_table[index].Rip_nexthop, 4)) { // next-hop�� ���� ���
					memcpy(&entry.ipAddress, pFrame->Rip_table[index].Rip_ipAddress, 4);
					entry.metric = metric;
					memcpy(&entry.dstInterface, routerDlg->m_IPLayer->GetSrcIP(dev_num), 4);
					CRouterDlg::route_table.SetAt(CRouterDlg::route_table.FindIndex(selectIndex), entry);
				} else {	// next-hop�� �ٸ� ���
					if (metric < entry.metric) {	// ���ο� metric���� �� ������ �װɷ� Update
						memcpy(&entry.ipAddress, pFrame->Rip_table[index].Rip_ipAddress, 4);
						entry.metric = metric;
						memcpy(&entry.dstInterface, routerDlg->m_IPLayer->GetSrcIP(dev_num), 4);
						CRouterDlg::route_table.SetAt(CRouterDlg::route_table.FindIndex(selectIndex), entry);
					} else {	// �̹� �����ϴ� metric���� �� ������ �״�� ��
						;
					}
				}
			} else {	// �ش� IP�� �������� ������ �״�� Routing table�� �߰�
				memcpy(&entry.ipAddress, pFrame->Rip_table[index].Rip_ipAddress, 4);
				entry.metric = metric;
				memcpy(&entry.dstInterface, routerDlg->m_IPLayer->GetSrcIP(dev_num), 4);
				CRouterDlg::route_table.AddTail(entry);
			}
		}
	
		routerDlg->UpdateRouteTable();
	}

	return true;
}

void CRIPLayer::ResetHeader()
{
	Rip_header.Rip_command = 0x01;    // request: 1, response: 2 
	Rip_header.Rip_version = 0x02;	  // version: 2
	Rip_header.Rip_reserved = 0x0000; // must be zero
}

void CRIPLayer::CreateRequestMessage()
{
	Rip_header.Rip_table[0].Rip_family = 0x0200;
	Rip_header.Rip_table[0].Rip_tag = 0x0100;
	memset(Rip_header.Rip_table[0].Rip_ipAddress,  0, 4);
	memset(Rip_header.Rip_table[0].Rip_subnetmask, 0, 4);
	memset(Rip_header.Rip_table[0].Rip_nexthop, 0, 4);
	Rip_header.Rip_table[0].Rip_metric = htonl(16); // Default metric : 16
}

void CRIPLayer::CreateResponseMessageTable()
{
	CRouterDlg::RoutingTable entry;
	int entries = CRouterDlg::route_table.GetCount();

	for (int index = 0; index < entries; index++) {
		entry = CRouterDlg::route_table.GetAt(CRouterDlg::route_table.FindIndex(index));

		Rip_header.Rip_table[index].Rip_family = 0x0200;
		Rip_header.Rip_table[index].Rip_tag = 0x0100;
		memcpy(Rip_header.Rip_table[index].Rip_ipAddress, entry.ipAddress, 4);
		memset(Rip_header.Rip_table[index].Rip_subnetmask, 0, 4);
		memset(Rip_header.Rip_table[index].Rip_nexthop, 0, 4);

		// Metric�� 1���� ������
		Rip_header.Rip_table[index].Rip_metric = htonl(entry.metric + 1);
	}
}

// MH: Route Table�� �ش� Entry�� �����ϴ��� Ȯ��, index�� ��ȯ�Ѵ�.
int CRIPLayer::ContainsRouteTableEntry(unsigned char Ip_addr[4]) 
{
	CRouterDlg::RoutingTable entry;
	int size = CRouterDlg::route_table.GetCount();

	if (size != 0) {
		for(int index = 0; index < size; index++) {
			entry = CRouterDlg::route_table.GetAt(CRouterDlg::route_table.FindIndex(index));

			// IP�� ��ġ�ϴ� Entry�� �����ϸ� �� index return.
			if(!memcmp(Ip_addr, entry.ipAddress,  4))
				return index;
		}
	}

	return -1;
}
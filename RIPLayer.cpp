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
		messageLength = 20;
	}

	if (nlength == 2) {
		Rip_header.Rip_command = 0x02;
		CreateResponseMessageTable();
		messageLength = CRouterDlg::route_table.GetSize() * 20;
	}

	routerDlg->m_EthernetLayer->SetDestinAddress(macbroadcast, dev_num);
	routerDlg->m_IPLayer->SetDstIP(broadcast, dev_num);
	routerDlg->m_UDPLayer->SetSrcPort(0x0802);
	BOOL bSuccess = mp_UnderLayer->Send((unsigned char*)&Rip_header, RIP_HEADER_SIZE + messageLength, dev_num);
	return bSuccess;
}

BOOL CRIPLayer::Receive(unsigned char* ppayload, int dev_num)
{
	CRouterDlg::RoutingTable entry;
	CRouterDlg* routerDlg = ((CRouterDlg *) GetUpperLayer(0));
	PRipHeader pFrame = (PRipHeader) ppayload;
	unsigned short length = routerDlg->m_UDPLayer->GetLength(dev_num) - 12;

	if (pFrame->Rip_command == 0x01) // command : request
		Send(0, 2, dev_num);

	if (pFrame->Rip_command == 0x02) { // command : response
		for (int index = 0; index < (length / 20); index++) {
			CRouterDlg::RoutingTable rt1;
			unsigned int metric = htonl(pFrame->Rip_table[index].Rip_metric);
			int selectIndex = ContainsRouteTableEntry(pFrame->Rip_table[index].Rip_ipAddress);

			if (selectIndex != -1) {
				entry = CRouterDlg::route_table.GetAt(CRouterDlg::route_table.FindIndex(selectIndex));
				 
				if (!memcmp((unsigned char*) routerDlg->m_IPLayer->GetSrcIP(2), pFrame->Rip_table[index].Rip_nexthop, 4)) {
					memcpy(&rt1.ipAddress, pFrame->Rip_table[index].Rip_ipAddress, 4);
					rt1.metric = metric;
					memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(dev_num), 4);
					CRouterDlg::route_table.SetAt(CRouterDlg::route_table.FindIndex(selectIndex), rt1);
				} else {
					if (metric < entry.metric) {
						memcpy(&rt1.ipAddress, pFrame->Rip_table[index].Rip_ipAddress, 4);
						rt1.metric = metric;
						memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(dev_num), 4);
						CRouterDlg::route_table.SetAt(CRouterDlg::route_table.FindIndex(selectIndex), rt1);
					} else {
						;
					}
				}
			} else {
				memcpy(&rt1.ipAddress, pFrame->Rip_table[index].Rip_ipAddress, 4);
				rt1.metric = metric;
				memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(dev_num), 4);
				CRouterDlg::route_table.AddTail(rt1);
			}
		}
	
		routerDlg->UpdateRouteTable();
	}

	return true;
}

void CRIPLayer::ResetHeader()
{
	Rip_header.Rip_command = 0x01; // request: 1, response: 2 
	Rip_header.Rip_version = 0x02; // version: 2
	Rip_header.Rip_reserved = 0x0000; // must be zero
}

void CRIPLayer::CreateRequestMessage()
{
	Rip_header.Rip_table[0].Rip_family = 0x0200;
	Rip_header.Rip_table[0].Rip_tag = 0x0100;
	memset(Rip_header.Rip_table[0].Rip_ipAddress,  0, 4);
	memset(Rip_header.Rip_table[0].Rip_subnetmask, 0, 4);
	memset(Rip_header.Rip_table[0].Rip_nexthop, 0, 4);
	Rip_header.Rip_table[0].Rip_metric = htonl(16);
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
		Rip_header.Rip_table[index].Rip_metric = htonl(entry.metric + 1);
	}
}

// MH: Route Table에 해당 Entry가 존재하는지 확인, index를 반환한다.
int CRIPLayer::ContainsRouteTableEntry(unsigned char Ip_addr[4]) 
{
	CRouterDlg::RoutingTable entry;
	int size = CRouterDlg::route_table.GetCount();

	if (size != 0) {
		for(int index = 0; index < size; index++) {
			entry = CRouterDlg::route_table.GetAt(CRouterDlg::route_table.FindIndex(index));

			if(!memcmp(Ip_addr, entry.ipAddress,  4))
				return index;
		}
	}

	return -1;
}
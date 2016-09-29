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

	if (nlength == 1) 
	{
		Rip_header.Rip_command = 0x01;
		CreateRequestMessage(dev_num);
		messageLength = 20;
	}
	else if (nlength == 2) 
	{
		Rip_header.Rip_command = 0x02;
		CreateResponseMessageTable(dev_num);
		if (dev_num == 1)
		{
			messageLength = CRouterDlg::route_table.GetSize() * 20;
		}
		else 
		{
			messageLength = CRouterDlg::route_table2.GetSize() * 20;
		}
	}

	routerDlg->m_EthernetLayer->SetDestinAddress(macbroadcast, dev_num);
	routerDlg->m_IPLayer->SetDstIP(broadcast, dev_num);
	routerDlg->m_UDPLayer->SetSrcPort(0x0802);
	BOOL bSuccess = mp_UnderLayer->Send((unsigned char*)&Rip_header, RIP_HEADER_SIZE + messageLength, dev_num);
	return bSuccess;
}

BOOL CRIPLayer::Receive(unsigned char* ppayload, int dev_num)
{
	POSITION index;
	CRouterDlg::RoutingTable entry;
	CRouterDlg * routerDlg =  ((CRouterDlg *)GetUpperLayer(0));
	PRipHeader pFrame = (PRipHeader) ppayload;

	if (pFrame->Rip_command == 0x01) { // command : request
		Send(0, 2, dev_num);
	}
	if (pFrame->Rip_command == 0x02) { // command : response
		unsigned short length = routerDlg->m_UDPLayer->GetLength(dev_num) - 8;
		
		if (dev_num == 1)
		{
			for (int i=0; i < (length/20); i++)
			{
				CRouterDlg::RoutingTable entry;
				CRouterDlg::RoutingTable rt1;
				unsigned int metric = htonl(pFrame->Rip_table[i].Rip_metric);
				int selectIndex = ContainsRouteTableEntry(pFrame->Rip_table[i].Rip_ipAddress, dev_num);

				if (selectIndex != -1) {
					entry = CRouterDlg::route_table.GetAt(CRouterDlg::route_table.FindIndex(selectIndex));

					if (!memcmp((unsigned char*)routerDlg->m_IPLayer->GetSrcIP(2), pFrame->Rip_table[i].Rip_nexthop, 4)) {
						memcpy(&rt1.ipAddress, pFrame->Rip_table[i].Rip_ipAddress, 4);
						rt1.metric = metric;
						memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(2), 4);
						CRouterDlg::route_table.SetAt(CRouterDlg::route_table.FindIndex(selectIndex), rt1);
					} else {
						if (metric < entry.metric)
						{
							memcpy(&rt1.ipAddress, pFrame->Rip_table[i].Rip_ipAddress, 4);
							rt1.metric = metric;
							memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(2), 4);
							CRouterDlg::route_table.SetAt(CRouterDlg::route_table.FindIndex(selectIndex), rt1);
						}
						else {
							;
						}
					}
				} else {
					memcpy(&rt1.ipAddress, pFrame->Rip_table[i].Rip_ipAddress, 4);
					rt1.metric = metric;
					memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(2), 4);
					CRouterDlg::route_table.AddTail(rt1);
				}
			}
		}
		if (dev_num == 2) 
		{
			for (int i=0; i < (length/20); i++)
			{
				CRouterDlg::RoutingTable entry;
				CRouterDlg::RoutingTable rt1;
				unsigned int metric = htonl(pFrame->Rip_table[i].Rip_metric);
				int selectIndex = ContainsRouteTableEntry(pFrame->Rip_table[i].Rip_ipAddress, dev_num);

				if (selectIndex != -1) {
					entry = CRouterDlg::route_table2.GetAt(CRouterDlg::route_table2.FindIndex(selectIndex));

					if (!memcmp((unsigned char*)routerDlg->m_IPLayer->GetSrcIP(2), pFrame->Rip_table[i].Rip_nexthop, 4)) {
						memcpy(&rt1.ipAddress, pFrame->Rip_table[i].Rip_ipAddress, 4);
						rt1.metric = metric;
						memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(1), 4);
						CRouterDlg::route_table2.SetAt(CRouterDlg::route_table.FindIndex(selectIndex), rt1);
					} else {
						if (metric < entry.metric)
						{
							memcpy(&rt1.ipAddress, pFrame->Rip_table[i].Rip_ipAddress, 4);
							rt1.metric = metric;
							memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(1), 4);
							CRouterDlg::route_table2.SetAt(CRouterDlg::route_table.FindIndex(selectIndex), rt1);
						}
						else {
							;
						}
					}
				} else {
					memcpy(&rt1.ipAddress, pFrame->Rip_table[i].Rip_ipAddress, 4);
					rt1.metric = metric;
					memcpy(&rt1.dstInterface, routerDlg->m_IPLayer->GetSrcIP(1), 4);
					CRouterDlg::route_table2.AddTail(rt1);
				}
			}
		}

		routerDlg->UpdateRouteTable(dev_num);
	}

	return true;
}

void CRIPLayer::ResetHeader()
{
	Rip_header.Rip_command = 0x01; // request: 1, response: 2 
	Rip_header.Rip_version = 0x02; // version: 2
	Rip_header.Rip_reserved = 0x0000; // must be zero
}

void CRIPLayer::CreateRequestMessage(int dev_num)
{
	Rip_header.Rip_table[0].Rip_family = 0x0200; // ip (2) 
	Rip_header.Rip_table[0].Rip_tag = 0x0100; // default AS (1)
	memset(Rip_header.Rip_table[0].Rip_ipAddress,  0x00000000, 4); // 수정필요
	memset(Rip_header.Rip_table[0].Rip_subnetmask, 0x00000000, 4); // ? 수정필요
	memset(Rip_header.Rip_table[0].Rip_nexthop, 0x00000000, 4); // ? 이론 공부 필요
	Rip_header.Rip_table[0].Rip_metric = htonl(16); // default (16) ? 수정필요
}

void CRIPLayer::CreateResponseMessageTable(int dev_num)
{
	POSITION index;
	CRouterDlg::RoutingTable entry;
	int messageLength = CRouterDlg::route_table.GetCount();
	int count = 0;

	if (dev_num == 1) {
	for (int i = 0; i<CRouterDlg::route_table.GetCount(); i++, count++){
		index = CRouterDlg::route_table.FindIndex(i);
		entry = CRouterDlg::route_table.GetAt(index);

		Rip_header.Rip_table[count].Rip_family = 0x0200; // ip (2) 
		Rip_header.Rip_table[count].Rip_tag = 0x0100; // default AS (1)
		memcpy(Rip_header.Rip_table[count].Rip_ipAddress, entry.ipAddress, 4); // 수정필요
		memset(Rip_header.Rip_table[count].Rip_subnetmask, 0x00000000, 4); // ? 수정필요
		memset(Rip_header.Rip_table[count].Rip_nexthop, 0x00000000, 4); // ? 이론 공부 필요
		Rip_header.Rip_table[count].Rip_metric = htonl(entry.metric + 1); // default (16) ? 수정필요
	}
	}

	if (dev_num == 2) {
	for (int i = 0; i<CRouterDlg::route_table2.GetCount(); i++, count++){
		index = CRouterDlg::route_table2.FindIndex(i);
		entry = CRouterDlg::route_table2.GetAt(index);

		Rip_header.Rip_table[count].Rip_family = 0x0200; // ip (2) 
		Rip_header.Rip_table[count].Rip_tag = 0x0100; // default AS (1)
		memcpy(Rip_header.Rip_table[count].Rip_ipAddress, entry.ipAddress, 4); // 수정필요
		memset(Rip_header.Rip_table[count].Rip_subnetmask, 0x00000000, 4); // ? 수정필요
		memset(Rip_header.Rip_table[count].Rip_nexthop, 0x00000000, 4); // ? 이론 공부 필요
		Rip_header.Rip_table[count].Rip_metric = htonl(entry.metric + 1); // default (16) ? 수정필요
	}
	}
}

void CRIPLayer::addMessageTable(PRipHeader pFrame, unsigned short length, int dev_num)
{
	
}

// MH: Route Table에 해당 Entry가 존재하는지 확인, index를 반환한다.
int CRIPLayer::ContainsRouteTableEntry(unsigned char Ip_addr[4], int dev_num) 
{
	int dev_1_count = CRouterDlg::route_table.GetCount();
	int dev_2_count = CRouterDlg::route_table2.GetCount();
	int i;
	CRouterDlg::RoutingTable entry;

	if (dev_num == 1) {
		if(dev_1_count == 0) {		//Cache table 에 아무것도 없을 경우
			return -1;
		} else {								//존재 할 경우
			for(i=0; i<dev_1_count; i++) {
				entry = CRouterDlg::route_table.GetAt(CRouterDlg::route_table.FindIndex(i));
				if(!memcmp(Ip_addr, entry.ipAddress,  4)) {
					return i;		// index 값 리턴
				}
			}
		}
	}

	if (dev_num == 2) {
		if(dev_2_count == 0) {		//Cache table 에 아무것도 없을 경우
			return -1;
		} else {								//존재 할 경우
			for(i=0; i<dev_2_count; i++) {
				entry = CRouterDlg::route_table2.GetAt(CRouterDlg::route_table2.FindIndex(i));
				if(!memcmp(Ip_addr, entry.ipAddress,  4)) {
					return i;		// index 값 리턴
				}
			}
		}
	}

	return -1;
}



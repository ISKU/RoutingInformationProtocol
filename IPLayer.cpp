#include "StdAfx.h"
#include "IPLayer.h"
#include "RouterDlg.h"

CIPLayer::CIPLayer(char* pName) : CBaseLayer(pName) 
{ 
	ResetHeader();
}

CIPLayer::~CIPLayer() 
{ 
}

void CIPLayer::SetDstIP(unsigned char* ip, int dev_num)
{
	if(dev_num == 1)
		memcpy(dev_1_dst_ip_addr, ip , 4 );
	else
		memcpy(dev_2_dst_ip_addr, ip , 4 );

	memcpy(&Ip_header.Ip_dstAddress,ip,4);
}

void CIPLayer::SetSrcIP(unsigned char* ip ,int dev_num)
{
	if(dev_num == 1)
		memcpy(dev_1_ip_addr , ip , 4 );
	else
		memcpy(dev_2_ip_addr , ip , 4 );

	memcpy(&Ip_header.Ip_srcAddress,ip,4);
}

void CIPLayer::SetSrcIPForRIPLayer(unsigned char* ip, int dev_num)
{
	if(dev_num == 1)
		memcpy(dev_1_ip_addr_for_rip , ip , 4 );
	else 
		memcpy(dev_2_ip_addr_for_rip , ip , 4 );
}

unsigned char* CIPLayer::GetDstIP(int dev_num)
{
	if(dev_num == 1)
		return dev_1_dst_ip_addr;
	return dev_2_dst_ip_addr;
}

unsigned char* CIPLayer::GetSrcIP(int dev_num)
{
	if(dev_num == 1)
		return dev_1_ip_addr;
	return dev_2_ip_addr;
}

unsigned char* CIPLayer::GetSrcIPForRIPLayer(int dev_num) 
{
	if(dev_num == 1)
		return dev_1_ip_addr_for_rip;
	return dev_2_ip_addr_for_rip;
}

unsigned char CIPLayer::GetProtocol(int dev_num) 
{
	if(dev_num == 1)
		return dev_1_protocol;
	return dev_2_protocol;
}

void CIPLayer::SetProtocol(unsigned char protocol, int dev_num)
{
	if(dev_num == 1)
		dev_1_protocol = protocol;
	else
		dev_2_protocol = protocol;

	Ip_header.Ip_protocol = protocol;
}

unsigned short CIPLayer::SetChecksum()
{
	unsigned char* p_header = (unsigned char*) &Ip_header;
	unsigned short word;
	unsigned int sum = 0;
	int i;

	for(i = 0; i < IP_HEADER_SIZE; i = i + 2) {
		if(i == 10) 
			continue;
		word = ((p_header[i] << 8) & 0xFF00) + (p_header[i+1] & 0xFF);
		sum = sum + (unsigned int) word;
	}

	while(sum >> 16)
		sum = (sum&0xFFFF) + (sum >> 16);

	sum = ~sum;
	return (unsigned short) sum;
}

BOOL CIPLayer::IsValidChecksum(unsigned char* p_header, unsigned short checksum)
{
	unsigned short word, ret;
	unsigned int sum = 0;
	int i;

	for(i = 0; i < IP_HEADER_SIZE; i = i + 2) {
		if(i == 10) continue;
		word = ((p_header[i] << 8) & 0xFF00) + (p_header[i+1] & 0xFF);
		sum = sum + (unsigned int) word;
	}

	while(sum >> 16)
		sum = (sum&0xFFFF) + (sum >> 16);

	ret = sum & checksum;
	return	ret == 0;
}

// ppayload == (unsigned char*)&Tcp_header
BOOL CIPLayer::Send(unsigned char* ppayload, int nlength, int dev_num)
{
	// IP 주소 셋팅은 Set 버튼을 눌렀을때 셋팅이 되었으므로 data와 전체 크기를 저장후 전송
	nlength = nlength + IP_HEADER_SIZE;
	Ip_header.Ip_len = (unsigned short) htons(nlength);
	memcpy(Ip_header.Ip_srcAddressByte, GetSrcIP(dev_num), 4);
	Ip_header.Ip_checksum = htons(SetChecksum());

	memcpy(Ip_header.Ip_data , ppayload , nlength);
	BOOL bSuccess = mp_UnderLayer->Send((unsigned char*) &Ip_header , nlength, dev_num);
	return bSuccess;
}

BOOL CIPLayer::Receive(unsigned char* ppayload,int dev_num)
{
	PIpHeader pFrame = (PIpHeader)ppayload;

	if(!memcmp(pFrame->Ip_srcAddressByte, GetSrcIP(dev_num), 4)) //자신이 보낸 패킷은 버린다
		return FALSE;

	if(!IsValidChecksum((unsigned char*) pFrame, ntohs(pFrame->Ip_checksum)))
		return FALSE;
	
	if (pFrame->Ip_protocol == 0x11) { // udp protocol (17) 확인
		SetSrcIPForRIPLayer(pFrame->Ip_srcAddressByte, dev_num);
		((CUDPLayer*)GetUpperLayer(0))->SetReceivePseudoHeader(pFrame->Ip_srcAddressByte, pFrame->Ip_dstAddressByte, (unsigned short) htons(ntohs(pFrame->Ip_len) - IP_HEADER_SIZE));
		return GetUpperLayer(0)->Receive((unsigned char *)pFrame->Ip_data, dev_num);
	}
	/*else { // else 부분 수정 필요 ( RIP가 아닌 일반 ip 패킷일 때 어떻게 해야할지)
	int dev = ((CRouterDlg *)GetUpperLayer(0))->Routing(pFrame->Ip_dstAddressByte);
	if(dev != -1){
	((CRouterDlg *)GetUpperLayer(0))->m_ARPLayer->Send((unsigned char *)pFrame,
	(int)htons(pFrame->Ip_len) + IP_HEADER_SIZE,dev);
	return TRUE;
	}
	}*/
	return TRUE;
}

void CIPLayer::ResetHeader( )
{
	Ip_header.Ip_version = 0x45; // version, header length
	Ip_header.Ip_typeOfService = 0x00;
	Ip_header.Ip_len = 0x0014; // 20byte default
	Ip_header.Ip_id = 0x0000;		
	Ip_header.Ip_fragmentOffset = 0x0000;
	Ip_header.Ip_timeToLive = 0x05; // ttl default
	Ip_header.Ip_protocol = 0x00; // setting
	Ip_header.Ip_checksum = 0x0000; 
	memset(Ip_header.Ip_srcAddressByte, 0, 4);
	memset(Ip_header.Ip_dstAddressByte, 0, 4);
	memset(Ip_header.Ip_data, 0, IP_MAX_DATA);
}

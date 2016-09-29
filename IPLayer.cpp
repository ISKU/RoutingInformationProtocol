#include "StdAfx.h"
#include "IPLayer.h"
#include "RouterDlg.h"
CIPLayer::CIPLayer( char* pName ) : CBaseLayer( pName ) 
{ 
	ResetHeader();
}

CIPLayer::~CIPLayer() { }

// ip1 setting function
void CIPLayer::SetDstIP( unsigned char* ip, int dev_num )
{
	if(dev_num == 1){
		memcpy(dev_1_dst_ip_addr, ip , 4 );
	}
	else{
		memcpy(dev_2_dst_ip_addr, ip , 4 );
	}
	memcpy(&Ip_header.Ip_dstAddress,ip,4);
}

void CIPLayer::SetSrcIP( unsigned char* ip ,int dev_num)
{
	if(dev_num == 1){
		memcpy(dev_1_ip_addr , ip , 4 );
	}
	else{
		memcpy(dev_2_ip_addr , ip , 4 );
	}
	memcpy(&Ip_header.Ip_srcAddress,ip,4);
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

unsigned char CIPLayer::GetProtocol(int dev_num) {
	if(dev_num == 1)
		return dev_1_protocol;
	return dev_2_protocol;
}

void CIPLayer::SetProtocol( unsigned char protocol, int dev_num )
{
	if(dev_num == 1){
		dev_1_protocol = protocol;
	}
	else{
		dev_2_protocol = protocol;
	}
	Ip_header.Ip_protocol = protocol;
}

// ppayload == (unsigned char*)&Tcp_header
BOOL CIPLayer::Send(unsigned char* ppayload, int nlength,int dev_num)
{
	// IP �ּ� ������ Set ��ư�� �������� ������ �Ǿ����Ƿ� data�� ��ü ũ�⸦ ������ ����
	nlength = IP_HEADER_SIZE + nlength;
	Ip_header.Ip_len = (unsigned short) htons(nlength);
	memcpy(Ip_header.Ip_srcAddressByte, GetSrcIP(dev_num), 4);

	memcpy(Ip_header.Ip_data , ppayload , nlength);
	BOOL bSuccess = mp_UnderLayer->Send((unsigned char*)&Ip_header , nlength, dev_num);
	return bSuccess;
}

BOOL CIPLayer::Receive(unsigned char* ppayload,int dev_num)
{
	PIpHeader pFrame = (PIpHeader)ppayload;

	if(!memcmp(&pFrame->Ip_srcAddressByte, GetSrcIP(dev_num), 4)) { //�ڽ��� ���� ��Ŷ�� ������
		return FALSE;
	}
	else if (pFrame->Ip_protocol == 0x11)  // udp protocol (17) Ȯ��
	{ 
		GetUpperLayer(0)->Receive((unsigned char *)pFrame->Ip_data, dev_num);
	}
	/*else { // else �κ� ���� �ʿ� ( RIP�� �ƴ� �Ϲ� ip ��Ŷ�� �� ��� �ؾ�����)
		int dev = ((CRouterDlg *)GetUpperLayer(0))->Routing(pFrame->Ip_dstAddressByte);
		if(dev != -1){
			((CRouterDlg *)GetUpperLayer(0))->m_ARPLayer->Send((unsigned char *)pFrame,
				(int)htons(pFrame->Ip_len) + IP_HEADER_SIZE,dev);
			return TRUE;
		}
	}*/
	return true;
}

void CIPLayer::ResetHeader( )
{
		//ip1 reset
		Ip_header.Ip_version = 0x45; // version, header length
		Ip_header.Ip_typeOfService = 0x00;
		Ip_header.Ip_len = 0x0014; // 20byte default
		Ip_header.Ip_id = 0x0000;		
		Ip_header.Ip_fragmentOffset = 0x0000;
		Ip_header.Ip_timeToLive = 0x05; // ttl default
		Ip_header.Ip_protocol = 0x00; // setting
		Ip_header.Ip_checksum = 0x0000; 
		memset(Ip_header.Ip_srcAddressByte , 0 , 4);
		memset(Ip_header.Ip_dstAddressByte , 0 , 4);
		memset(Ip_header.Ip_data , 0 , IP_MAX_DATA);
}
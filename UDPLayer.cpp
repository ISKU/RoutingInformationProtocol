#include "StdAfx.h"
#include "UDPLayer.h"
#include "RouterDlg.h"
CUDPLayer::CUDPLayer(char* pName) : CBaseLayer(pName)
{
	ResetHeader();
}

CUDPLayer::~CUDPLayer() { }

unsigned short CUDPLayer::GetSrcPort()
{
	return Udp_header.Udp_srcPort;
}

unsigned short CUDPLayer::GetDstPort()
{
	return Udp_header.Udp_dstPort;
}

unsigned short CUDPLayer::GetLength(int dev_num)
{
	if(dev_num == 1)
		return dev_1_length;
	return dev_2_length;
}

void CUDPLayer::SetSrcPort(unsigned short port)
{
	Udp_header.Udp_srcPort = port;
}

void CUDPLayer::SetDstPort(unsigned short port)
{
	Udp_header.Udp_dstPort = port;
}

void CUDPLayer::SetLength(unsigned short length, int dev_num)
{
	if(dev_num == 1)
		dev_1_length = length;
	else
		dev_2_length = length;
}

BOOL CUDPLayer::Send(unsigned char* ppayload, int nlength, int dev_num)
{
	CRouterDlg * routerDlg =  ((CRouterDlg *)GetUpperLayer(0)->GetUpperLayer(0));

	memcpy(Udp_header.Udp_data, ppayload, nlength);
	nlength = UDP_HEADER_SIZE + nlength;
	routerDlg->m_IPLayer->SetProtocol(0x11, dev_num);

	Udp_header.Udp_length = (unsigned short) htons(nlength);
	BOOL bSuccess = mp_UnderLayer->Send((unsigned char*)&Udp_header, nlength, dev_num);
	return bSuccess;
}

BOOL CUDPLayer::Receive(unsigned char* ppayload, int dev_num)
{
	PUdpHeader pFrame = (PUdpHeader) ppayload;
	BOOL bSuccess = false;

	if (pFrame->Udp_checksum) {
		;// checksums calculator
	}

	if (pFrame->Udp_dstPort == 0x0802) { // check dst port 520
		SetLength((unsigned short) htons(pFrame->Udp_length), dev_num);
		bSuccess = GetUpperLayer(0)->Receive((unsigned char *)pFrame->Udp_data, dev_num);
	}
	return bSuccess;
}

void CUDPLayer::ResetHeader()
{
	Udp_header.Udp_srcPort = 0x0802; // int 520 
	Udp_header.Udp_dstPort = 0x0802; // int 520
	Udp_header.Udp_length = 0x8; // header + data length (default udp header length)
	Udp_header.Udp_checksum = 0x00; // checksum
}
#pragma once
#include "baselayer.h"

class CUDPLayer : public CBaseLayer
{
public:
	CUDPLayer(char* pName = NULL);
	virtual ~CUDPLayer();

	unsigned short	GetDstPort();
	unsigned short	GetSrcPort();
	unsigned short	GetLength(int dev_num);
	void			SetDstPort(unsigned short port);
	void			SetSrcPort(unsigned short port);
	void			SetLength(unsigned short length, int dev_num);


	BOOL Send(unsigned char* ppayload, int nlength, int dev_num);
	BOOL Receive(unsigned char* ppayload, int dev_num);

public:
	typedef struct _UDP {
		unsigned short		Udp_srcPort;
		unsigned short		Udp_dstPort;
		unsigned short		Udp_length;
		unsigned short		Udp_checksum;
		unsigned char		Udp_data[UDP_MAX_DATA];
	}UdpHeader, *PUdpHeader;

private:
	inline void		ResetHeader();
	unsigned short dev_1_length;
	unsigned short dev_2_length;
	UdpHeader Udp_header;					
};
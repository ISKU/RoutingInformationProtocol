// RouterDlg.cpp : ���� ����
//
#include "stdafx.h"
#include "Router.h"
#include "RouterDlg.h"
#include "RoutTableAdder.h"
#include "ProxyTableAdder.h"
#include "IPLayer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// MH: 2���� route_table ����! ( RouterDlg.h�� static���� �����Ͽ� route_table ����� Ŭ���� �ܺο� ���� ��ü�� Dialog�� ������ �ִ�)
CList<CRouterDlg::RoutingTable, CRouterDlg::RoutingTable&> CRouterDlg::route_table;
//CList<CRouterDlg::RoutingTable, CRouterDlg::RoutingTable&> CRouterDlg::route_table2;
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// ���� ���α׷� ������ ���Ǵ� CAboutDlg ��ȭ �����Դϴ�.
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();


// ��ȭ ���� �������Դϴ�.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV �����Դϴ�.

// �����Դϴ�.
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{

}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CRouterDlg ��ȭ ����
CRouterDlg::CRouterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRouterDlg::IDD, pParent), CBaseLayer("CRouterDlg")
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//listIndex = -1;
	//ProxyListIndex = -1;
	// Layer ����
	m_NILayer = new CNILayer("NI");
	m_EthernetLayer = new CEthernetLayer("Ethernet");
	m_ARPLayer = new CARPLayer("ARP");
	m_IPLayer = new CIPLayer("IP");
	m_UDPLayer = new CUDPLayer("UDP"); // MH: UDP Layer
	m_RIPLayer = new CRIPLayer("RIP"); // MH: RIP Layer

	//// Layer �߰�										
	m_LayerMgr.AddLayer( this );				
	m_LayerMgr.AddLayer( m_NILayer );			
	m_LayerMgr.AddLayer( m_EthernetLayer );
	m_LayerMgr.AddLayer( m_ARPLayer );
	m_LayerMgr.AddLayer( m_IPLayer );
	m_LayerMgr.AddLayer( m_UDPLayer ); // MH: UDP Layer �߰�
	m_LayerMgr.AddLayer( m_RIPLayer ); // MH: RIP LAyer �߰�

	//Layer���� ///////////////////////////////////////////////////////////////////////////
	m_NILayer->SetUpperLayer(m_EthernetLayer);
	m_EthernetLayer->SetUpperLayer(m_IPLayer);
	m_EthernetLayer->SetUpperLayer(m_ARPLayer);
	m_EthernetLayer->SetUnderLayer(m_NILayer);
	m_ARPLayer->SetUnderLayer(m_EthernetLayer);
	m_ARPLayer->SetUpperLayer(m_IPLayer);
	m_IPLayer->SetUpperLayer(m_UDPLayer);
	m_IPLayer->SetUnderLayer(m_ARPLayer);
	m_UDPLayer->SetUpperLayer(m_RIPLayer);
	m_UDPLayer->SetUnderLayer(m_IPLayer);
	m_RIPLayer->SetUpperLayer(this);
	m_RIPLayer->SetUnderLayer(m_UDPLayer);
	this->SetUnderLayer(m_RIPLayer);
	// MH: ���Ӱ� �߰��� UDP, RIP Layer�� ���� Ethernet-> ARP -> IP -> UDP -> RIP -> Dialog

	//Layer ����
}

void CRouterDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_ROUTING_TABLE, ListBox_RoutingTable);
	DDX_Control(pDX, IDC_CACHE_TABLE, ListBox_ARPCacheTable);
	DDX_Control(pDX, IDC_PROXY_TABLE, ListBox_ARPProxyTable);
	DDX_Control(pDX, IDC_NIC1_COMBO, m_nic1);
	DDX_Control(pDX, IDC_NIC2_COMBO, m_nic2);
	DDX_Control(pDX, IDC_IPADDRESS1, m_nic1_ip);
	DDX_Control(pDX, IDC_IPADDRESS2, m_nic2_ip);
}

BEGIN_MESSAGE_MAP(CRouterDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_CACHE_DELETE, &CRouterDlg::OnBnClickedCacheDelete)
	ON_BN_CLICKED(IDC_CACHE_DELETE_ALL, &CRouterDlg::OnBnClickedCacheDeleteAll)
	ON_BN_CLICKED(IDC_PROXY_DELETE, &CRouterDlg::OnBnClickedProxyDelete)
	ON_BN_CLICKED(IDC_PROXY_DELETE_ALL, &CRouterDlg::OnBnClickedProxyDeleteAll)
	ON_BN_CLICKED(IDC_PROXY_ADD, &CRouterDlg::OnBnClickedProxyAdd)
	ON_BN_CLICKED(IDCANCEL, &CRouterDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_NIC_SET_BUTTON, &CRouterDlg::OnBnClickedNicSetButton)
	ON_BN_CLICKED(IDC_ROUTING_ADD, &CRouterDlg::OnBnClickedRoutingAdd)
	ON_BN_CLICKED(IDC_ROUTING_DELETE, &CRouterDlg::OnBnClickedRoutingDelete)
	ON_CBN_SELCHANGE(IDC_NIC1_COMBO, &CRouterDlg::OnCbnSelchangeNic1Combo)
	ON_CBN_SELCHANGE(IDC_NIC2_COMBO, &CRouterDlg::OnCbnSelchangeNic2Combo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ROUTING_TABLE, &CRouterDlg::OnLvnItemchangedRoutingTable)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_ROUTING_TABLE2, &CRouterDlg::OnLvnItemchangedRoutingTable2)
END_MESSAGE_MAP()


// CRouterDlg �޽��� ó����

BOOL CRouterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// �ý��� �޴��� "����..." �޴� �׸��� �߰��մϴ�.

	// IDM_ABOUTBOX�� �ý��� ��� ������ �־�� �մϴ�.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// �� ��ȭ ������ �������� �����մϴ�. ���� ���α׷��� �� â�� ��ȭ ���ڰ� �ƴ� ��쿡��
	//  �����ӿ�ũ�� �� �۾��� �ڵ����� �����մϴ�.
	SetIcon(m_hIcon, TRUE);			// ū �������� �����մϴ�.
	SetIcon(m_hIcon, FALSE);		// ���� �������� �����մϴ�.

	// TODO: ���⿡ �߰� �ʱ�ȭ �۾��� �߰��մϴ�.
	// ListBox�� �ʱ� Colum�� ����
	ListBox_RoutingTable.InsertColumn(0, _T(" "), LVCFMT_CENTER, 20, -1);
	ListBox_RoutingTable.InsertColumn(1,_T("IP Address"),LVCFMT_CENTER,175,-1);
	ListBox_RoutingTable.InsertColumn(2,_T("Metric"),LVCFMT_CENTER,166,-1);
	ListBox_RoutingTable.InsertColumn(3,_T("Interface"),LVCFMT_CENTER,175,-1);

	ListBox_ARPCacheTable.InsertColumn(0,_T("IP address"),LVCFMT_CENTER,100,-1);
	ListBox_ARPCacheTable.InsertColumn(1,_T("Mac address"),LVCFMT_CENTER,120,-1);
	ListBox_ARPCacheTable.InsertColumn(2,_T("Type"),LVCFMT_CENTER,80,-1);
	//ListBox_ARPCacheTable.InsertColumn(3,_T("Time"),LVCFMT_CENTER,49,-1);

	ListBox_ARPProxyTable.InsertColumn(0,_T("Name"),LVCFMT_CENTER,60,-1);
	ListBox_ARPProxyTable.InsertColumn(1,_T("IP address"),LVCFMT_CENTER,120,-1);
	ListBox_ARPProxyTable.InsertColumn(2,_T("Mac address"),LVCFMT_CENTER,120,-1);

	setNicList(); //NicList Setting
	return TRUE;  // ��Ŀ���� ��Ʈ�ѿ� �������� ������ TRUE�� ��ȯ�մϴ�.
}

void CRouterDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ��ȭ ���ڿ� �ּ�ȭ ���߸� �߰��� ��� �������� �׸�����
//  �Ʒ� �ڵ尡 �ʿ��մϴ�. ����/�� ���� ����ϴ� MFC ���� ���α׷��� ��쿡��
//  �����ӿ�ũ���� �� �۾��� �ڵ����� �����մϴ�.

void CRouterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // �׸��⸦ ���� ����̽� ���ؽ�Ʈ

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Ŭ���̾�Ʈ �簢������ �������� ����� ����ϴ�.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// �������� �׸��ϴ�.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ����ڰ� �ּ�ȭ�� â�� ���� ���ȿ� Ŀ���� ǥ�õǵ��� �ý��ۿ���
//  �� �Լ��� ȣ���մϴ�.
HCURSOR CRouterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRouterDlg::OnBnClickedCacheDelete()
{
	//CacheDeleteAll��ư
	int index = -1;
	index = ListBox_ARPCacheTable.GetSelectionMark();
	if(index != -1){
		POSITION pos = m_ARPLayer->Cache_Table.FindIndex(index);
		m_ARPLayer->Cache_Table.RemoveAt(pos);
		m_ARPLayer->updateCacheTable();
	}
}

void CRouterDlg::OnBnClickedCacheDeleteAll()
{
	//CacheDeleteAll��ư
	m_ARPLayer->Cache_Table.RemoveAll();
	m_ARPLayer->updateCacheTable();
}
void CRouterDlg::OnBnClickedProxyDelete()
{
	//proxy delete��ư
	m_ARPLayer->Proxy_Table.RemoveAll();
	m_ARPLayer->updateProxyTable();
}

void CRouterDlg::OnBnClickedProxyDeleteAll()
{
	//proxy delete all ��ư
	int index = -1;
	index = ListBox_ARPProxyTable.GetSelectionMark();
	if(index != -1){
		POSITION pos = m_ARPLayer->Proxy_Table.FindIndex(index);
		m_ARPLayer->Proxy_Table.RemoveAt(pos);
		m_ARPLayer->updateProxyTable();
	}
}

void CRouterDlg::OnBnClickedProxyAdd()
{
	// proxy add ��ư
	CString str;
	unsigned char Ip[4];
	unsigned char Mac[8];
	ProxyTableAdder PDlg;
	if(	PDlg.DoModal() == IDOK)
	{
		str = PDlg.getName();
		memcpy(Ip , PDlg.getIp() , 4);
		memcpy(Mac , PDlg.getMac() , 6);

//		m_ARPLayer->InsertProxy(str,Ip,Mac);
	}
}

void CRouterDlg::OnBnClickedCancel()
{
	// ���� ��ư
	exit(0);
}

void CRouterDlg::OnBnClickedNicSetButton()
{
	LPADAPTER adapter = NULL;		// ��ī�忡 ���� ������ �����ϴ� pointer ����
	PPACKET_OID_DATA OidData;
	pcap_if_t *Devices;

	OidData = (PPACKET_OID_DATA)malloc(sizeof(PACKET_OID_DATA));
	OidData->Oid = 0x01010101;
	OidData->Length = 6;

	ZeroMemory(OidData->Data,6);
	char DeviceName1[512];
	char DeviceName2[512];
	char strError[30];
	if(pcap_findalldevs_ex( PCAP_SRC_IF_STRING, NULL , &Devices , strError) != 0) {
		printf("pcap_findalldevs_ex() error : %s\n", strError);
	}

	m_nic1.GetLBText(m_nic1.GetCurSel() , DeviceName1);	// �޺� �ڽ��� ���õ� Device�� �̸��� ����
	m_nic2.GetLBText(m_nic2.GetCurSel() , DeviceName2);
	while(Devices != NULL) {
		if(!strcmp(Devices->description,DeviceName1))
			Device1 = Devices;
		if(!strcmp(Devices->description,DeviceName2))
			Device2 = Devices;
		Devices = Devices->next;
	}

	// device����
	m_NILayer->SetDevice(Device1,1);
	m_NILayer->SetDevice(Device2,2);
	RtDlg.setDeviceList(Device1->description,Device2->description);

	//mac �ּ� ����
	adapter = PacketOpenAdapter((Device1->name+8));
	PacketRequest( adapter, FALSE, OidData);
	adapter = PacketOpenAdapter((Device2->name+8));
	PacketRequest( adapter, FALSE, OidData);


	//ip�ּ� ����
	unsigned char nic1_ip[4];
	unsigned char nic2_ip[4];
	m_nic1_ip.GetAddress((BYTE &)nic1_ip[0],(BYTE &)nic1_ip[1],(BYTE &)nic1_ip[2],(BYTE &)nic1_ip[3]);
	m_nic2_ip.GetAddress((BYTE &)nic2_ip[0],(BYTE &)nic2_ip[1],(BYTE &)nic2_ip[2],(BYTE &)nic2_ip[3]);


	// MH: �ڱ� �ڽ��� ����� ���� ������Ʈ //////////////////////////////////
	RoutingTable rt1;
	memcpy(&rt1.ipAddress, nic1_ip, 4);
	rt1.metric = 0x0;
	memcpy(&rt1.dstInterface, nic1_ip, 4);

	RoutingTable rt2;
	memcpy(&rt2.ipAddress, nic2_ip, 4);
	rt2.metric = 0x0;
	memcpy(&rt2.dstInterface, nic2_ip, 4);

	route_table.AddTail(rt1);
	route_table.AddTail(rt2);
	UpdateRouteTable();
	////////////////////////////////////////////////////////////////////////


	m_NILayer->StartReadThread();	// receive Thread start
	GetDlgItem(IDC_NIC_SET_BUTTON)->EnableWindow(0);

	// MH: ����� ���ý� request rip packet ����////////////////////////////
	unsigned char broadcast[4];
	memset(broadcast,0xff,4);
	unsigned char macbroadcast[6];
	memset(macbroadcast,0xff,6);
	
	m_EthernetLayer->SetDestinAddress(macbroadcast, 1);
	m_EthernetLayer->SetDestinAddress(macbroadcast, 2);
	m_IPLayer->SetDstIP(broadcast, 1);
	m_IPLayer->SetDstIP(broadcast, 2);
	m_IPLayer->SetSrcIP(nic1_ip, 1);
	m_IPLayer->SetSrcIP(nic2_ip, 2);
	m_EthernetLayer->SetSourceAddress(OidData->Data,1);
	m_EthernetLayer->SetSourceAddress(OidData->Data,2);

	m_RIPLayer->Send(1, 1);
	m_RIPLayer->Send(1, 2);
	StartReadThread(); // MH: RIP Response Thread start 30��
	//////////////////////////////////////////////////////////////////////

}


void CRouterDlg::OnBnClickedRoutingAdd()
{
/*	//router Table Add��ư
	if( RtDlg.DoModal() == IDOK ){
		RoutingTable rt;

		memcpy(&rt.Destnation,RtDlg.GetDestIp(),6);
		rt.Flag = RtDlg.GetFlag();
		memcpy(&rt.Gateway,RtDlg.GetGateway(),6);
		memcpy(&rt.Netmask,RtDlg.GetNetmask(),6);
		rt.Interface = RtDlg.GetInterface();
		rt.Metric = RtDlg.GetMetric();

		route_table.AddTail(rt);
		UpdateRouteTable();
	}
	*/
}

void CRouterDlg::OnBnClickedRoutingDelete()
{
	/*
	// router Table delete��ư
	int index = -1;
	index = ListBox_RoutingTable.GetSelectionMark();
	if(index != -1){
		POSITION pos = route_table.FindIndex(index);
		route_table.RemoveAt(pos);
		UpdateRouteTable();
	}
	*/
}

// NicList Set
void CRouterDlg::setNicList(void)
{
	pcap_if_t *Devices;
	char strError[30];
	if(pcap_findalldevs_ex( PCAP_SRC_IF_STRING, NULL , &Devices , strError) != 0) {
		printf("pcap_findalldevs_ex() error : %s\n", strError);
	}

	//set device_1
	while(Devices != NULL) {
		m_nic1.AddString(Devices->description);
		m_nic2.AddString(Devices->description);
		Devices = Devices->next;
	}
	m_nic1.SetCurSel(0);
	m_nic2.SetCurSel(1);
}	

/* not used
void CRouterDlg::add_route_table(unsigned char ip[4], unsigned int metric, unsigned char dstInterface[4])
{
	RoutingTable rt;
	memcpy(&rt.IpAddressByte, ip, 4);
	rt.Metric = metric;
	memcpy(&rt.Interface, dstInterface, 4);
}
*/

// UpdateRouteTable
void CRouterDlg::UpdateRouteTable()
{
	RoutingTable entry;
	CString tableNumber, ipAddress, metric, dstInterface;
	int size = route_table.GetCount();

	// MH: dev_num���� �����Ͽ� interface�� �ش��ϴ� route_table�� ����Ͽ� CList�� �ִ� entry�� ��� ���̾ƿ��� �߰��Ѵ�!
	ListBox_RoutingTable.DeleteAllItems();
	for(int index = 0; index < size; index++) {
		entry = route_table.GetAt(route_table.FindIndex(index));

		tableNumber.Format("%d", index + 1);
		ipAddress.Format("%d.%d.%d.%d", entry.ipAddress[0], entry.ipAddress[1], entry.ipAddress[2], entry.ipAddress[3]);
		metric.Format("%d", entry.metric);
		dstInterface.Format("%d.%d.%d.%d", entry.dstInterface[0], entry.dstInterface[1], entry.dstInterface[2], entry.dstInterface[3]);
		
		ListBox_RoutingTable.InsertItem(index, tableNumber);
		ListBox_RoutingTable.SetItem(index, 1, LVIF_TEXT, ipAddress, 0, 0, 0, NULL);
		ListBox_RoutingTable.SetItem(index, 2, LVIF_TEXT, metric, 0, 0, 0, NULL);
		ListBox_RoutingTable.SetItem(index, 3, LVIF_TEXT, dstInterface, 0, 0, 0, NULL);
		ListBox_RoutingTable.UpdateWindow();
	}
}

int CRouterDlg::Routing(unsigned char destip[4]) {
	/*
	POSITION index;
	RoutingTable entry;
	RoutingTable select_entry;
	entry.Interface = -2;
	select_entry.Interface = -2;
	unsigned char result[4];
	for(int i=0; i<route_table.GetCount(); i++) {
		index = route_table.FindIndex(i);
		entry = route_table.GetAt(index);

		// select_entry�� �������� �ʴ� ��� 
		if(select_entry.Interface == -2){
			for(int j=0; j<4; j++)
				result[j] = destip[j] & entry.Netmask[j];

			// destination�� ���� ��� 
			if(!memcmp(result,entry.Destnation,4)){ 

				// gateway�� ������ ��� 
				if(((entry.Flag & 0x01) == 0x01) && ((entry.Flag & 0x02) == 0x02)){ 
					select_entry = entry;
					m_IPLayer->SetDstIP(entry.Gateway);
				}

				// gateway�� �ƴ� ��� 
				else if(((entry.Flag & 0x01) == 0x01) && ((entry.Flag & 0x02) == 0x00)){ 
					select_entry = entry;
					m_IPLayer->SetDstIP(destip);
				}
			}
		}
		// �����ϴ� ��� 
		else { 
			for(int j=0; j<4; j++)
				result[j] = destip[j] & entry.Netmask[j];

			// ���� select��Ʈ ���� 1�� ������ ���� ��� 
			if(memcmp(result,entry.Netmask,4)){ 
				for(int j=0; j<4; j++)
					result[j] = destip[j] & entry.Netmask[j];

				// destation�� ���� ��� 
				if(!memcmp(result,entry.Destnation,4)){ 

					// gateway�� ������ ��� 
					if(((entry.Flag & 0x01) == 0x01) && ((entry.Flag & 0x02) == 0x02)){ 
						select_entry = entry;
						m_IPLayer->SetDstIP(entry.Gateway);
					}

					// gateway�� �ƴ� ���
					else if(((entry.Flag & 0x01) == 0x01) && ((entry.Flag & 0x02) == 0x00)){ 
						select_entry = entry;
						m_IPLayer->SetDstIP(destip);
					}
				}
			}
			// �� ���� ��� pass 
		}
	}
	return select_entry.Interface+1;
	*/
	return true;
}

void CRouterDlg::OnCbnSelchangeNic1Combo()
{// ip�ּ� ����
}

void CRouterDlg::OnCbnSelchangeNic2Combo()
{ //ip �ּ� ����
}

void CRouterDlg::OnLvnItemchangedRoutingTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	*pResult = 0;
}


void CRouterDlg::OnLvnItemchangedRoutingTable2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: ���⿡ ��Ʈ�� �˸� ó���� �ڵ带 �߰��մϴ�.
	*pResult = 0;
}

// MH: 30�ʸ��� RIP ���� ��Ŷ�� ������ ////////////////////////////////////////////////
void CRouterDlg::StartReadThread()
{
	pThread_1 = AfxBeginThread(WaitRipResponseMessagePacket_1 , this);
	pThread_2 = AfxBeginThread(WaitRipResponseMessagePacket_2 , this);
	if(pThread_1 == NULL || pThread_2 == NULL) {
		AfxMessageBox("Read ������ ���� ����");
	}
}

unsigned int CRouterDlg::WaitRipResponseMessagePacket_1(LPVOID pParam) 
{
	CRouterDlg *temp_CRouterDlgLayer = (CRouterDlg*)pParam;

	while(1) {
		Sleep(7000);
		temp_CRouterDlgLayer->GetUnderLayer()->Send(0, 2, 1);
	}

	return 0;
}

unsigned int CRouterDlg::WaitRipResponseMessagePacket_2(LPVOID pParam){
	CRouterDlg *temp_CRouterDlgLayer = (CRouterDlg*)pParam;
	
	while(1) {
		Sleep(7000);
		temp_CRouterDlgLayer->GetUnderLayer()->Send(0, 2, 2);
	}

	return 0;
}
//////////////////////////////////////////////////////////////////////////////////// END
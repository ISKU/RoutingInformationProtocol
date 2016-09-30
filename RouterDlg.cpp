// RouterDlg.cpp : 구현 파일
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

// MH: 2개의 route_table 선언! ( RouterDlg.h에 static으로 선언하여 route_table 멤버는 클래스 외부에 선언 본체는 Dialog가 가지고 있다)
CList<CRouterDlg::RoutingTable, CRouterDlg::RoutingTable&> CRouterDlg::route_table;
//CList<CRouterDlg::RoutingTable, CRouterDlg::RoutingTable&> CRouterDlg::route_table2;
//////////////////////////////////////////////////////////////////////////////////////////////////////////

// 응용 프로그램 정보에 사용되는 CAboutDlg 대화 상자입니다.
class CAboutDlg : public CDialog
{
public:
	CAboutDlg();


// 대화 상자 데이터입니다.
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 지원입니다.

// 구현입니다.
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


// CRouterDlg 대화 상자
CRouterDlg::CRouterDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRouterDlg::IDD, pParent), CBaseLayer("CRouterDlg")
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	//listIndex = -1;
	//ProxyListIndex = -1;
	// Layer 생성
	m_NILayer = new CNILayer("NI");
	m_EthernetLayer = new CEthernetLayer("Ethernet");
	m_ARPLayer = new CARPLayer("ARP");
	m_IPLayer = new CIPLayer("IP");
	m_UDPLayer = new CUDPLayer("UDP"); // MH: UDP Layer
	m_RIPLayer = new CRIPLayer("RIP"); // MH: RIP Layer

	//// Layer 추가										
	m_LayerMgr.AddLayer( this );				
	m_LayerMgr.AddLayer( m_NILayer );			
	m_LayerMgr.AddLayer( m_EthernetLayer );
	m_LayerMgr.AddLayer( m_ARPLayer );
	m_LayerMgr.AddLayer( m_IPLayer );
	m_LayerMgr.AddLayer( m_UDPLayer ); // MH: UDP Layer 추가
	m_LayerMgr.AddLayer( m_RIPLayer ); // MH: RIP LAyer 추가

	//Layer연결 ///////////////////////////////////////////////////////////////////////////
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
	// MH: 새롭게 추가된 UDP, RIP Layer를 연결 Ethernet-> ARP -> IP -> UDP -> RIP -> Dialog

	//Layer 생성
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


// CRouterDlg 메시지 처리기

BOOL CRouterDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// 시스템 메뉴에 "정보..." 메뉴 항목을 추가합니다.

	// IDM_ABOUTBOX는 시스템 명령 범위에 있어야 합니다.
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

	// 이 대화 상자의 아이콘을 설정합니다. 응용 프로그램의 주 창이 대화 상자가 아닐 경우에는
	//  프레임워크가 이 작업을 자동으로 수행합니다.
	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	// TODO: 여기에 추가 초기화 작업을 추가합니다.
	// ListBox에 초기 Colum을 삽입
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
	return TRUE;  // 포커스를 컨트롤에 설정하지 않으면 TRUE를 반환합니다.
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

// 대화 상자에 최소화 단추를 추가할 경우 아이콘을 그리려면
//  아래 코드가 필요합니다. 문서/뷰 모델을 사용하는 MFC 응용 프로그램의 경우에는
//  프레임워크에서 이 작업을 자동으로 수행합니다.

void CRouterDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// 사용자가 최소화된 창을 끄는 동안에 커서가 표시되도록 시스템에서
//  이 함수를 호출합니다.
HCURSOR CRouterDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CRouterDlg::OnBnClickedCacheDelete()
{
	//CacheDeleteAll버튼
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
	//CacheDeleteAll버튼
	m_ARPLayer->Cache_Table.RemoveAll();
	m_ARPLayer->updateCacheTable();
}
void CRouterDlg::OnBnClickedProxyDelete()
{
	//proxy delete버튼
	m_ARPLayer->Proxy_Table.RemoveAll();
	m_ARPLayer->updateProxyTable();
}

void CRouterDlg::OnBnClickedProxyDeleteAll()
{
	//proxy delete all 버튼
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
	// proxy add 버튼
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
	// 종료 버튼
	exit(0);
}

void CRouterDlg::OnBnClickedNicSetButton()
{
	LPADAPTER adapter = NULL;		// 랜카드에 대한 정보를 저장하는 pointer 변수
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

	m_nic1.GetLBText(m_nic1.GetCurSel() , DeviceName1);	// 콤보 박스에 선택된 Device의 이름을 얻어옴
	m_nic2.GetLBText(m_nic2.GetCurSel() , DeviceName2);
	while(Devices != NULL) {
		if(!strcmp(Devices->description,DeviceName1))
			Device1 = Devices;
		if(!strcmp(Devices->description,DeviceName2))
			Device2 = Devices;
		Devices = Devices->next;
	}

	// device설정
	m_NILayer->SetDevice(Device1,1);
	m_NILayer->SetDevice(Device2,2);
	RtDlg.setDeviceList(Device1->description,Device2->description);

	//mac 주소 설정
	adapter = PacketOpenAdapter((Device1->name+8));
	PacketRequest( adapter, FALSE, OidData);
	adapter = PacketOpenAdapter((Device2->name+8));
	PacketRequest( adapter, FALSE, OidData);


	//ip주소 설정
	unsigned char nic1_ip[4];
	unsigned char nic2_ip[4];
	m_nic1_ip.GetAddress((BYTE &)nic1_ip[0],(BYTE &)nic1_ip[1],(BYTE &)nic1_ip[2],(BYTE &)nic1_ip[3]);
	m_nic2_ip.GetAddress((BYTE &)nic2_ip[0],(BYTE &)nic2_ip[1],(BYTE &)nic2_ip[2],(BYTE &)nic2_ip[3]);


	// MH: 자기 자신의 라우팅 정보 업데이트 //////////////////////////////////
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

	// MH: 라우터 부팅시 request rip packet 전달////////////////////////////
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
	StartReadThread(); // MH: RIP Response Thread start 30초
	//////////////////////////////////////////////////////////////////////

}


void CRouterDlg::OnBnClickedRoutingAdd()
{
/*	//router Table Add버튼
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
	// router Table delete버튼
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

	// MH: dev_num으로 구분하여 interface에 해당하는 route_table을 사용하여 CList에 있는 entry를 모두 레이아웃에 추가한다!
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

		// select_entry가 존재하지 않는 경우 
		if(select_entry.Interface == -2){
			for(int j=0; j<4; j++)
				result[j] = destip[j] & entry.Netmask[j];

			// destination이 같은 경우 
			if(!memcmp(result,entry.Destnation,4)){ 

				// gateway로 보내는 경우 
				if(((entry.Flag & 0x01) == 0x01) && ((entry.Flag & 0x02) == 0x02)){ 
					select_entry = entry;
					m_IPLayer->SetDstIP(entry.Gateway);
				}

				// gateway가 아닌 경우 
				else if(((entry.Flag & 0x01) == 0x01) && ((entry.Flag & 0x02) == 0x00)){ 
					select_entry = entry;
					m_IPLayer->SetDstIP(destip);
				}
			}
		}
		// 존재하는 경우 
		else { 
			for(int j=0; j<4; j++)
				result[j] = destip[j] & entry.Netmask[j];

			// 기존 select비트 보다 1의 개수가 많은 경우 
			if(memcmp(result,entry.Netmask,4)){ 
				for(int j=0; j<4; j++)
					result[j] = destip[j] & entry.Netmask[j];

				// destation이 같은 경우 
				if(!memcmp(result,entry.Destnation,4)){ 

					// gateway로 보내는 경우 
					if(((entry.Flag & 0x01) == 0x01) && ((entry.Flag & 0x02) == 0x02)){ 
						select_entry = entry;
						m_IPLayer->SetDstIP(entry.Gateway);
					}

					// gateway가 아닌 경우
					else if(((entry.Flag & 0x01) == 0x01) && ((entry.Flag & 0x02) == 0x00)){ 
						select_entry = entry;
						m_IPLayer->SetDstIP(destip);
					}
				}
			}
			// 더 적을 경우 pass 
		}
	}
	return select_entry.Interface+1;
	*/
	return true;
}

void CRouterDlg::OnCbnSelchangeNic1Combo()
{// ip주소 설정
}

void CRouterDlg::OnCbnSelchangeNic2Combo()
{ //ip 주소 설정
}

void CRouterDlg::OnLvnItemchangedRoutingTable(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}


void CRouterDlg::OnLvnItemchangedRoutingTable2(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	*pResult = 0;
}

// MH: 30초마다 RIP 응답 패킷을 보낸다 ////////////////////////////////////////////////
void CRouterDlg::StartReadThread()
{
	pThread_1 = AfxBeginThread(WaitRipResponseMessagePacket_1 , this);
	pThread_2 = AfxBeginThread(WaitRipResponseMessagePacket_2 , this);
	if(pThread_1 == NULL || pThread_2 == NULL) {
		AfxMessageBox("Read 쓰레드 생성 실패");
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
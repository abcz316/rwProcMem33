// NetworkDlg.cpp: 实现文件
//

#include "pch.h"
#include "testHwBpClient.h"
#include "NetworkDlg.h"
#include "afxdialogex.h"
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <IPHlpApi.h>
#pragma comment(lib,"IPHlpApi.lib")
#include<WINSOCK2.H>
#pragma comment(lib,"ws2_32.lib")
#include "Global.h"

#define WM_ADD_FIND_SERVER WM_USER + 1

// CNetworkDlg 对话框

IMPLEMENT_DYNAMIC(CNetworkDlg, CDialogEx)

CNetworkDlg::CNetworkDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_NETWORK_DIALOG, pParent)
	, m_edit_ip(_T(""))
	, m_edit_port(_T("")) {

}

CNetworkDlg::~CNetworkDlg() {
}

void CNetworkDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_FIND_SERVER, m_list_find_server);
	DDX_Text(pDX, IDC_EDIT_IP, m_edit_ip);
	DDX_Text(pDX, IDC_EDIT_PORT, m_edit_port);
}


BEGIN_MESSAGE_MAP(CNetworkDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_CONNECT, &CNetworkDlg::OnBnClickedConnect)
	ON_BN_CLICKED(IDC_CANCEL, &CNetworkDlg::OnBnClickedCancel)
	ON_MESSAGE(WM_ADD_FIND_SERVER, OnAddFindServer)
	ON_NOTIFY(HDN_ITEMCLICK, 0, &CNetworkDlg::OnItemclickListFindServer)
	ON_NOTIFY(NM_CLICK, IDC_LIST_FIND_SERVER, &CNetworkDlg::OnClickListFindServer)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_FIND_SERVER, &CNetworkDlg::OnDblclkListFindServer)
END_MESSAGE_MAP()

// CNetworkDlg 消息处理程序

//测试服务器端口是否开放
void TestIdentifierThread(std::unique_ptr<char[]> ip, HWND resultHwnd) {
	SOCKET skServer = socket(PF_INET, SOCK_DGRAM, 0);
	sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_port = htons(3290);
	addr.sin_addr.S_un.S_addr = inet_addr(ip.get());
	BOOL bSuccess = (connect(skServer, (sockaddr*)&addr, sizeof(sockaddr_in)) == 0) ? TRUE : FALSE;

	if (bSuccess && IsWindow(resultHwnd)) {
#pragma pack(1)
		struct {
			uint32_t checksum;
			uint16_t port;
		} packet;
#pragma pack()

		struct sockaddr_in addr_client = { 0 };
		addr_client.sin_family = PF_INET;
		addr_client.sin_addr.s_addr = inet_addr(ip.get());
		addr_client.sin_port = htons(3290);

		socklen_t clisize = sizeof(addr_client);

		packet.checksum = 1;
		packet.port = 3290;
		if (sendto(skServer, (const char*)&packet, sizeof(packet), 0, (struct sockaddr *)&addr_client, (int)&clisize) > 0) {
			if (recvfrom(skServer, (char*)&packet, sizeof(packet), 0, (struct sockaddr *)&addr_client, &clisize) > 0) {
				if (packet.checksum == 0xce) {
					//找到testHwBpServer了，通知主界面显示
					::PostMessage(resultHwnd, WM_ADD_FIND_SERVER, (WPARAM)ip.release(), packet.port);
				}
			}
		}
	}
	closesocket(skServer);

	return;
}
//探测可连接的服务器地址
BOOL GetHwServerIP(HWND resultHwnd) {
	//IP_ADAPTER_INFO结构体
	PIP_ADAPTER_INFO pIpAdapterInfo = NULL;
	pIpAdapterInfo = new IP_ADAPTER_INFO;

	//结构体大小
	unsigned long ulSize = sizeof(IP_ADAPTER_INFO);

	//获取适配器信息
	int nRet = GetAdaptersInfo(pIpAdapterInfo, &ulSize);

	if (ERROR_BUFFER_OVERFLOW == nRet) {
		//空间不足，删除之前分配的空间
		delete[]pIpAdapterInfo;

		//重新分配大小
		pIpAdapterInfo = (PIP_ADAPTER_INFO) new BYTE[ulSize];

		//获取适配器信息
		nRet = GetAdaptersInfo(pIpAdapterInfo, &ulSize);

		//获取失败
		if (ERROR_SUCCESS != nRet) {
			if (pIpAdapterInfo != NULL) {
				delete[]pIpAdapterInfo;
			}
			return FALSE;
		}
	}

	//MAC 地址信息
	//char szMacAddr[20];
	//赋值指针
	PIP_ADAPTER_INFO pIterater = pIpAdapterInfo;
	while (pIterater) {
		//cout << "网卡名称：" << pIterater->AdapterName << endl;

		//cout << "网卡描述：" << pIterater->Description << endl;

		/*sprintf_s(szMacAddr, 20, "%02X-%02X-%02X-%02X-%02X-%02X",
			pIterater->Address[0],
			pIterater->Address[1],
			pIterater->Address[2],
			pIterater->Address[3],
			pIterater->Address[4],
			pIterater->Address[5]);*/

			//cout << "MAC 地址：" << szMacAddr << endl;

			//cout << "IP地址列表：" << endl << endl;

			//指向IP地址列表
		PIP_ADDR_STRING pIpAddr = &pIterater->IpAddressList;
		while (pIpAddr) {
			//cout << "IP地址：  " << pIpAddr->IpAddress.String << endl;
			//cout << "子网掩码：" << pIpAddr->IpMask.String << endl;

			//指向网关列表
			PIP_ADDR_STRING pGateAwayList = &pIterater->GatewayList;
			while (pGateAwayList) {
				//cout << "网关：    " << pGateAwayList->IpAddress.String << endl;

				if (strcmp(pGateAwayList->IpAddress.String, "0.0.0.0") != 0) {
					//测试是否可连接
					std::unique_ptr<char[]> temIP = std::make_unique<char[]>(256);
					memset(temIP.get(), 0, 256);
					strcpy(temIP.get(), pGateAwayList->IpAddress.String);

					std::thread td(TestIdentifierThread, std::move(temIP), resultHwnd);
					td.detach();
				}

				pGateAwayList = pGateAwayList->Next;
			}

			pIpAddr = pIpAddr->Next;
		}
		//cout << endl << "--------------------------------------------------" << endl;

		pIterater = pIterater->Next;
	}

	//清理
	if (pIpAdapterInfo) {
		delete[]pIpAdapterInfo;
	}



	return TRUE;
}

BOOL CNetworkDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化


	//初始化列表框
	LONG lStyle;
	lStyle = GetWindowLong(m_list_find_server.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;
	SetWindowLong(m_list_find_server.m_hWnd, GWL_STYLE, lStyle);
	DWORD dwStyle = m_list_find_server.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl）
	//dwStyle |= LVS_EX_CHECKBOXES;//item前生成checkbox控件
	m_list_find_server.SetExtendedStyle(dwStyle);

	m_list_find_server.InsertColumn(0, L"IP", LVCFMT_LEFT, 200);//插入列
	m_list_find_server.InsertColumn(1, L"端口", LVCFMT_LEFT, 200);

	//探测可连接的服务器地址
	GetHwServerIP(m_hWnd);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


BOOL CNetworkDlg::PreTranslateMessage(MSG* pMsg) {
	// TODO: 在此添加专用代码和/或调用基类

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CNetworkDlg::OnClose() {
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnClose();
}


void CNetworkDlg::OnDestroy() {
	CDialogEx::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
}


void CNetworkDlg::OnSize(UINT nType, int cx, int cy) {
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
}


void CNetworkDlg::OnTimer(UINT_PTR nIDEvent) {
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnTimer(nIDEvent);
}


void CNetworkDlg::OnBnClickedConnect() {
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(TRUE);
	if (m_edit_ip.IsEmpty() || m_edit_port.IsEmpty()) {
		MessageBox(L"IP或端口不能为空");
		return;
	}
	if (!g_NetworkManager.ConnectHwBpServer(ws2s(m_edit_ip.GetBuffer(0)), _wtoi(m_edit_port.GetBuffer(0)))) {
		MessageBox(L"连接失败");
		return;
	}
	OnOK();
}


void CNetworkDlg::OnBnClickedCancel() {
	OnOK();
}

LRESULT CNetworkDlg::OnAddFindServer(WPARAM wParam, LPARAM lParam) {
	std::unique_ptr<char[]> myMsgParam((char*)wParam);

	//显示IP地址
	int nCount = m_list_find_server.GetItemCount();
	m_list_find_server.InsertItem(nCount, s2ws(myMsgParam.get()).c_str());

	//显示端口号
	std::wstringstream ssConvert;
	ssConvert << (size_t)lParam;
	m_list_find_server.SetItemText(nCount, 1, ssConvert.str().c_str());

	return 0;
}


void CNetworkDlg::OnItemclickListFindServer(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMHEADER phdr = reinterpret_cast<LPNMHEADER>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CNetworkDlg::OnClickListFindServer(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate->iItem != -1) {
		//按下了列表
		m_edit_ip = m_list_find_server.GetItemText(pNMItemActivate->iItem, 0);
		m_edit_port = m_list_find_server.GetItemText(pNMItemActivate->iItem, 1);
		UpdateData(FALSE);
	}
	*pResult = 0;
}


void CNetworkDlg::OnDblclkListFindServer(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate->iItem != -1) {
		//按下了列表
		m_edit_ip = m_list_find_server.GetItemText(pNMItemActivate->iItem, 0);
		m_edit_port = m_list_find_server.GetItemText(pNMItemActivate->iItem, 1);
		UpdateData(FALSE);
		//开始连接
		OnBnClickedConnect();
	}
	*pResult = 0;
}

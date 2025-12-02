
// testHwBpClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "testHwBpClient.h"
#include "testHwBpClientDlg.h"
#include "afxdialogex.h"
#include "TextViewDlg.h"
#include <sstream>
#include <algorithm>
#include <thread>
#include "../testHwBp/jni/HwBreakpointMgr4.h"
#include "../testHwBpServer/jni/hwbpserver.h"
#include "ScaleHelper.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx {
public:
	CAboutDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX) {
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CtestHwBpClientDlg 对话框



CtestHwBpClientDlg::CtestHwBpClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TESTHWBPCLIENT_DIALOG, pParent)
	, m_edit_pid(_T(""))
	, m_edit_addr(_T(""))
	, m_edit_keep_time(_T(""))
	, m_checkbox_pid_hex(FALSE)
	, m_checkbox_addr_hex(FALSE)
	, m_radio_type_r(FALSE)
	, m_radio_len_4(FALSE)
	, m_radio_region_all_thread(FALSE) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CtestHwBpClientDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_RESULT, m_list_result);
	DDX_Text(pDX, IDC_EDIT_PID, m_edit_pid);
	DDX_Text(pDX, IDC_EDIT_ADDR, m_edit_addr);
	DDX_Text(pDX, IDC_EDIT_KEEP_TIME, m_edit_keep_time);
	DDX_Check(pDX, IDC_CHECK_PID_HEX, m_checkbox_pid_hex);
	DDX_Check(pDX, IDC_CHECK_ADDR_HEX, m_checkbox_addr_hex);
	DDX_Radio(pDX, IDC_RADIO_TYPE_R, m_radio_type_r);
	DDX_Radio(pDX, IDC_RADIO_LEN_4, m_radio_len_4);
	DDX_Radio(pDX, IDC_RADIO_REGION_ALL_THREAD, m_radio_region_all_thread);
}

BEGIN_MESSAGE_MAP(CtestHwBpClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_ADD_HWBP, &CtestHwBpClientDlg::OnBnClickedButtonAddHwbp)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_RESULT, &CtestHwBpClientDlg::OnDblclkListResult)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_RESULT, &CtestHwBpClientDlg::OnRclickListResult)
	ON_COMMAND(ID_MENUITEM_CLEAN_LIST, &CtestHwBpClientDlg::OnMenuitemCleanList)
	ON_COMMAND(ID_MENUITEM_DELETE_SELECTED_COUNT, &CtestHwBpClientDlg::OnMenuitemDeleteSelectedCount)
	ON_COMMAND(ID_MENUITEM_DELETE_OTHER_COUNT, &CtestHwBpClientDlg::OnMenuitemDeleteOtherCount)
	ON_COMMAND(ID_MENUITEM_DELETE_UP_COUNT, &CtestHwBpClientDlg::OnMenuitemDeleteUpCount)
	ON_COMMAND(ID_MENUITEM_DELETE_DOWN_COUNT, &CtestHwBpClientDlg::OnMenuitemDeleteDownCount)
	ON_WM_SIZE()
	ON_COMMAND(ID_MENUITEM_COPY_SELECTED, &CtestHwBpClientDlg::OnMenuitemCopySelected)
	ON_COMMAND(ID_MENUITEM_COPY_LIST, &CtestHwBpClientDlg::OnMenuitemCopyList)
END_MESSAGE_MAP()


// CtestHwBpClientDlg 消息处理程序

BOOL CtestHwBpClientDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr) {
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	//初始化列表框
	LONG lStyle;
	lStyle = GetWindowLong(m_list_result.m_hWnd, GWL_STYLE);
	lStyle &= ~LVS_TYPEMASK; //清除显示方式位
	lStyle |= LVS_REPORT;
	SetWindowLong(m_list_result.m_hWnd, GWL_STYLE, lStyle);
	DWORD dwStyle = m_list_result.GetExtendedStyle();
	dwStyle |= LVS_EX_FULLROWSELECT;//选中某行使整行高亮（只适用与report风格的listctrl）
	dwStyle |= LVS_EX_GRIDLINES;//网格线（只适用与report风格的listctrl）
	//dwStyle |= LVS_EX_CHECKBOXES;//item前生成checkbox控件
	m_list_result.SetExtendedStyle(dwStyle);

	m_list_result.InsertColumn(HIT_RESULT_COL_ID, L"ID", LVCFMT_LEFT, GetScaleWidth(35));
	m_list_result.InsertColumn(HIT_RESULT_COL_TIME, L"时间", LVCFMT_LEFT, GetScaleWidth(135));
	m_list_result.InsertColumn(HIT_RESULT_COL_PID, L"PID", LVCFMT_LEFT, GetScaleWidth(50));
	m_list_result.InsertColumn(HIT_RESULT_COL_TID, L"线程ID", LVCFMT_LEFT, GetScaleWidth(50));
	m_list_result.InsertColumn(HIT_RESULT_COL_THREAD_RANGE, L"范围", LVCFMT_LEFT, GetScaleWidth(40));
	m_list_result.InsertColumn(HIT_RESULT_COL_BP_ADDR, L"进程地址", LVCFMT_LEFT, GetScaleWidth(130));
	m_list_result.InsertColumn(HIT_RESULT_COL_BP_TYPE, L"类型", LVCFMT_LEFT, GetScaleWidth(35));
	m_list_result.InsertColumn(HIT_RESULT_COL_BP_LEN, L"长度", LVCFMT_LEFT, GetScaleWidth(35));
	m_list_result.InsertColumn(HIT_RESULT_COL_HIT_ADDR, L"硬断命中地址", LVCFMT_LEFT, GetScaleWidth(130));
	m_list_result.InsertColumn(HIT_RESULT_COL_HIT_TOTAL_COUNT, L"总命中次数", LVCFMT_LEFT, GetScaleWidth(75));
	m_list_result.InsertColumn(HIT_RESULT_COL_HIT_INFO, L"命中信息", LVCFMT_LEFT, GetScaleWidth(1500));

	m_edit_pid = L"0";
	m_edit_addr = L"0";
	m_checkbox_addr_hex = TRUE;
	m_edit_keep_time = L"1000";
	m_radio_type_r = 0;
	m_radio_len_4 = 0;
	m_radio_region_all_thread = 0;
	UpdateData(FALSE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CtestHwBpClientDlg::OnSysCommand(UINT nID, LPARAM lParam) {
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CtestHwBpClientDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CtestHwBpClientDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

int CtestHwBpClientDlg::GetInputPid() {
	UpdateData(TRUE);
	std::wstringstream ssConvert;
	ssConvert << m_edit_pid.GetString();
	int pid = 0;
	if (m_checkbox_pid_hex) {
		ssConvert >> std::hex >> pid;
	} else {
		ssConvert >> pid;
	}
	return pid;
}
UINT64 CtestHwBpClientDlg::GetInputAddress() {
	UpdateData(TRUE);
	std::wstringstream ssConvert;
	ssConvert << m_edit_addr.GetString();
	UINT64 address = 0;
	if (m_checkbox_addr_hex) {
		ssConvert >> std::hex >> address;
	} else {
		ssConvert >> address;
	}
	return address;
}
std::wstring CtestHwBpClientDlg::GetInputAddressString() {
	UpdateData(TRUE);
	std::wstringstream wssConvert;
	if (m_checkbox_addr_hex) {
		wssConvert << std::hex << m_edit_addr.GetString();
	} else {
		wssConvert << m_edit_addr.GetString();
	}
	std::wstring showAddr = m_edit_addr.GetString();
	return showAddr;
}
uint32_t CtestHwBpClientDlg::GetInputHwBpAddrLen() {
	UpdateData(TRUE);
	return m_radio_len_4 == 0 ? HW_BREAKPOINT_LEN_4 : HW_BREAKPOINT_LEN_8;
}
uint32_t CtestHwBpClientDlg::GetInputHwBpAddrType() {
	UpdateData(TRUE);
	return m_radio_type_r == 0 ? HW_BREAKPOINT_R : m_radio_type_r == 1 ? HW_BREAKPOINT_W : m_radio_type_r == 2 ? HW_BREAKPOINT_RW : HW_BREAKPOINT_X;
}
uint32_t CtestHwBpClientDlg::GetInputHwBpKeepTimeMs() {
	UpdateData(TRUE);

	std::wstringstream ssConvert;
	uint32_t hwBpKeepTimeMs = 0;
	ssConvert << m_edit_keep_time.GetString();
	ssConvert >> hwBpKeepTimeMs;
	return hwBpKeepTimeMs;
}
void CtestHwBpClientDlg::OnBnClickedButtonAddHwbp() {
	UpdateData(TRUE);
	if (m_edit_pid.IsEmpty() || m_edit_addr.IsEmpty() || m_edit_keep_time.IsEmpty()) {
		MessageBox(L"所填参数不能为空");
		return;
	}

	int pid = GetInputPid();
	UINT64 address = GetInputAddress();
	if (pid == 0 || address == 0) {
		MessageBox(L"所填参数有错误");
		return;
	}
	if (!g_NetworkMgr.Reconnect()) {
		MessageBox(L"与服务器已失去连接");
		return;
	}

	//硬件断点类型
	InstProcessHwBpInfo input;
	InstProcessHwBpResult output;

	input.pid = GetInputPid();
	input.address = GetInputAddress();
	input.hwBpAddrLen = GetInputHwBpAddrLen();
	input.hwBpAddrType = GetInputHwBpAddrType();
	input.hwBpThreadType = m_radio_region_all_thread;
	input.hwBpKeepTimeMs = GetInputHwBpKeepTimeMs();

	//开始安装硬件断点
	g_NetworkMgr.InstProcessHwBp(input, output);

	//将命中结果显示到列表框
	int nIndex = m_list_result.GetItemCount();
	for (const InstProcessHwBpResultChild & threadInfo : output.vThreadHit) {
		for (const HW_HIT_ITEM& hitItem : threadInfo.vHitItem) {
			m_list_result.InsertItem(nIndex, m_edit_pid);

			std::wstring showAddr = GetInputAddressString();
			transform(showAddr.begin(), showAddr.end(), showAddr.begin(), ::toupper);

			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_ID, std::to_wstring(nIndex).c_str());
			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_TIME, TimestampToDatetime(hitItem.hit_time).c_str());
			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_PID, std::to_wstring(input.pid).c_str());
			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_TID, std::to_wstring(hitItem.task_id).c_str());
			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_BP_ADDR, std::wstring(L"0x" + showAddr).c_str());
			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_BP_TYPE, m_radio_type_r == 0 ? L"读" : m_radio_type_r == 1 ? L"写" : m_radio_type_r == 2 ? L"读写" : L"执行");
			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_BP_LEN, m_radio_len_4 == 0 ? L"4" : L"8");
			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_THREAD_RANGE, m_radio_region_all_thread == 0 ? L"全部" : m_radio_region_all_thread == 1 ? L"主线程" : L"其他线程");

			std::wstringstream wssConvert;
			wssConvert << std::hex << hitItem.hit_addr;
			showAddr = wssConvert.str();
			transform(showAddr.begin(), showAddr.end(), showAddr.begin(), ::toupper);

			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_HIT_ADDR, std::wstring(L"0x" + showAddr).c_str());

			std::wstring strHitTotalCount = std::to_wstring(threadInfo.hitTotalCount);
			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_HIT_TOTAL_COUNT, strHitTotalCount.c_str());

			std::wstringstream ssInfoText;
			wchar_t info[4096] = { 0 };
			for (int r = 0; r < 30; r += 5) {
				memset(info, 0, sizeof(info));
				wsprintf(info, L"X%-2d=%-20p X%-2d=%-20p X%-2d=%-20p X%-2d=%-20p X%-2d=%-20p\r\n",
					r, hitItem.regs_info.regs[r],
					r + 1, hitItem.regs_info.regs[r + 1],
					r + 2, hitItem.regs_info.regs[r + 2],
					r + 3, hitItem.regs_info.regs[r + 3],
					r + 4, hitItem.regs_info.regs[r + 4]);
				ssInfoText << info;
			}
			memset(info, 0, sizeof(info));
			wsprintf(info, L"\r\nLR= %-20p SP= %-20p PC= %-20p\r\n\r\n",
				hitItem.regs_info.regs[30],
				hitItem.regs_info.sp,
				hitItem.regs_info.pc);
			ssInfoText << info;

			memset(info, 0, sizeof(info));
			wsprintf(info, L"process status: %-20p orig_x0: %-20p\r\nsyscallno: %-20p\r\n",
				hitItem.regs_info.pstate,
				hitItem.regs_info.orig_x0,
				hitItem.regs_info.syscallno);

			ssInfoText << info;

			m_list_result.SetItemText(nIndex, HIT_RESULT_COL_HIT_INFO, ssInfoText.str().c_str());

			//使刚刚插入的新项可见
			m_list_result.EnsureVisible(nIndex, FALSE);
			nIndex++;
		}
	}
	g_NetworkMgr.Disconnect();

	if (output.allTaskCount == 0) {
		MessageBox(L"目标进程的线程数为0");
	} else if (output.allTaskCount != output.hwbpInstalledCount) {
		std::wstringstream wssBuf;
		wssBuf << L"发现有硬件断点安装失败的线程！目标进程的线程总数:" << output.allTaskCount << L", 硬件断点安装成功线程数:" << output.hwbpInstalledCount;
		MessageBox(wssBuf.str().c_str());
	}
}


void CtestHwBpClientDlg::OnDblclkListResult(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate->iItem != -1) {
		//按下了列表
		CString strHitAddr = m_list_result.GetItemText(pNMItemActivate->iItem, HIT_RESULT_COL_HIT_ADDR);
		CString strHitTotalCount = m_list_result.GetItemText(pNMItemActivate->iItem, HIT_RESULT_COL_HIT_TOTAL_COUNT);
		CString title;
		title.Format(L"命中地址:%s，总命中次数:%s", strHitAddr, strHitTotalCount);
		
		CString text = title;
		text += "\r\n";
		text += m_list_result.GetItemText(pNMItemActivate->iItem, HIT_RESULT_COL_HIT_INFO);
		text += "\r\n\r\n提示：\r\nR13（SP）：堆栈指针寄存器\r\nR14（LR）：子程序的返回地址";

		//显示窗口
		std::thread td([](std::shared_ptr<std::wstring>title, std::shared_ptr<std::wstring>text)->void {
			CTextViewDlg dlg(title->c_str(), text->c_str());
			dlg.DoModal();
		}, std::make_shared<std::wstring>(title.GetString()), std::make_shared<std::wstring>(text.GetString()));
		td.detach();
	}
	*pResult = 0;
}

std::wstring CtestHwBpClientDlg::TimestampToDatetime(uint64_t timestamp) {
	time_t t = (time_t)timestamp;
	struct tm* tm_info = localtime(&t);
	wchar_t buffer[256] = { 0 };
	wcsftime(buffer, 26, L"%Y-%m-%d %H:%M:%S", tm_info);
	return buffer;
}

void CtestHwBpClientDlg::OnRclickListResult(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);

	//加载菜单
	CMenu m_Menu;
	m_Menu.LoadMenu(IDR_MENU_LIST);
	//获取鼠标位置
	CPoint ptMouse;
	GetCursorPos(&ptMouse);

	if (pNMItemActivate->iItem == -1) {
		m_Menu.GetSubMenu(0)->RemoveMenu(ID_MENUITEM_DELETE_SELECTED_COUNT, MF_BYCOMMAND);
		m_Menu.GetSubMenu(0)->RemoveMenu(ID_MENUITEM_DELETE_OTHER_COUNT, MF_BYCOMMAND);
		m_Menu.GetSubMenu(0)->RemoveMenu(ID_MENUITEM_DELETE_UP_COUNT, MF_BYCOMMAND);
		m_Menu.GetSubMenu(0)->RemoveMenu(ID_MENUITEM_DELETE_DOWN_COUNT, MF_BYCOMMAND);
	}

	//显示菜单
	m_Menu.GetSubMenu(0)->TrackPopupMenu(
		TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
		ptMouse.x,
		ptMouse.y,
		this);
	*pResult = 0;

}


void CtestHwBpClientDlg::OnMenuitemCleanList() {
	m_list_result.DeleteAllItems();
}


void CtestHwBpClientDlg::OnMenuitemDeleteSelectedCount() {
	std::vector<int> vDeleteIndex;
	POSITION pos = m_list_result.GetFirstSelectedItemPosition(); //返回第一个选中的行位置
	if (pos != NULL) {
		while (pos) {
			int n = m_list_result.GetNextSelectedItem(pos);  //返回下一个选中的行数(返回值从0开始)
			//做相应操作
			vDeleteIndex.push_back(n);
		}
	}
	int deleted = 0;
	for (int n : vDeleteIndex) {
		n -= deleted;
		m_list_result.DeleteItem(n);
		deleted++;
	}

}


void CtestHwBpClientDlg::OnMenuitemDeleteOtherCount() {
	std::vector<int> vSaveIndex;
	POSITION pos = m_list_result.GetFirstSelectedItemPosition(); //返回第一个选中的行位置
	if (pos != NULL) {
		while (pos) {
			int n = m_list_result.GetNextSelectedItem(pos);  //返回下一个选中的行数(返回值从0开始)
			//做相应操作
			vSaveIndex.push_back(n);
		}
	}

	std::vector<int> vDeleteIndex;
	for (int i = 0; i < m_list_result.GetItemCount(); i++) {
		bool bSave = false;
		for (int saveIndex : vSaveIndex) {
			if (saveIndex == i) {
				bSave = true;
				break;
			}
		}
		if (!bSave) {
			vDeleteIndex.push_back(i);
		}
	}
	int deleted = 0;
	for (int n : vDeleteIndex) {
		n -= deleted;
		m_list_result.DeleteItem(n);
		deleted++;
	}

}


void CtestHwBpClientDlg::OnMenuitemDeleteUpCount() {
	int nFirstIndex = -1;
	POSITION pos = m_list_result.GetFirstSelectedItemPosition(); //返回第一个选中的行位置
	if (pos != NULL) {
		while (pos) {
			nFirstIndex = m_list_result.GetNextSelectedItem(pos);  //返回下一个选中的行数(返回值从0开始)
			break;
		}
	}
	if (nFirstIndex == -1) {
		return;
	}
	for (int i = 0; i < nFirstIndex; i++) {
		m_list_result.DeleteItem(0);
	}
}


void CtestHwBpClientDlg::OnMenuitemDeleteDownCount() {
	int nFirstIndex = -1;
	POSITION pos = m_list_result.GetFirstSelectedItemPosition(); //返回第一个选中的行位置
	if (pos != NULL) {
		while (pos) {
			nFirstIndex = m_list_result.GetNextSelectedItem(pos);  //返回下一个选中的行数(返回值从0开始)
			break;
		}
	}
	if (nFirstIndex == -1) {
		return;
	}
	int deleteCount = m_list_result.GetItemCount() - 1 - nFirstIndex;
	for (int i = 0; i < deleteCount; i++) {
		m_list_result.DeleteItem(nFirstIndex + 1);
	}
}


void CtestHwBpClientDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
}


void CtestHwBpClientDlg::OnMenuitemCopySelected()
{
	std::vector<int> vCopyIndex;
	POSITION pos = m_list_result.GetFirstSelectedItemPosition(); //返回第一个选中的行位置
	if (pos != NULL) {
		while (pos) {
			int n = m_list_result.GetNextSelectedItem(pos);  //返回下一个选中的行数(返回值从0开始)
			//做相应操作
			vCopyIndex.push_back(n);
		}
	}

	CString result;
	CString itemText;

	for (int row : vCopyIndex) {
		int columnCount = m_list_result.GetHeaderCtrl()->GetItemCount();
		for (int col = 0; col < columnCount; ++col) {
			itemText = m_list_result.GetItemText(row, col);
			// 添加到结果字符串，每个单元格的文本后面加一个空格作为分隔
			result += itemText;
			if (col < columnCount - 1)  // 最后一列后不加空格
				result += _T(" ");
		}
		result += _T("\n");
	}
	PutTextToClipboard(result.GetString());
}


void CtestHwBpClientDlg::OnMenuitemCopyList()
{
	CString result;
	CString itemText;

	// 获取列表控件的行数和列数
	int rowCount = m_list_result.GetItemCount();
	int columnCount = m_list_result.GetHeaderCtrl()->GetItemCount();

	// 遍历所有行和列
	for (int row = 0; row < rowCount; ++row) {
		for (int col = 0; col < columnCount; ++col) {
			itemText = m_list_result.GetItemText(row, col);
			// 添加到结果字符串，每个单元格的文本后面加一个空格作为分隔
			result += itemText;
			if (col < columnCount - 1)  // 最后一列后不加空格
				result += _T(" ");
		}
		result += _T("\n");
	}
	PutTextToClipboard(result.GetString());
}

BOOL CtestHwBpClientDlg::PutTextToClipboard(LPCTSTR pTxtData) {
	BOOL bRet = FALSE;
	if (OpenClipboard()) {
		EmptyClipboard();
		HGLOBAL hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
			(lstrlen(pTxtData) + 1) * sizeof(TCHAR));
		if (hData != NULL) {
			LPTSTR pszData = (LPTSTR)::GlobalLock(hData);
			lstrcpy(pszData, pTxtData);
			GlobalUnlock(hData);
#ifdef _UNICODE
			bRet = (SetClipboardData(CF_UNICODETEXT, hData) != NULL);
#else
			bRet = (SetClipboardData(CF_TEXT, hData) != NULL);
#endif // _UNICODE
		}
		CloseClipboard();
	}
	return bRet;
}
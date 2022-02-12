
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
	, m_radio_region_all_thread(FALSE)
	, m_checkbox_x0(FALSE)
	, m_checkbox_x1(FALSE)
	, m_checkbox_x2(FALSE)
	, m_checkbox_x3(FALSE)
	, m_checkbox_x4(FALSE)
	, m_checkbox_x5(FALSE)
	, m_checkbox_x6(FALSE)
	, m_checkbox_x7(FALSE)
	, m_checkbox_x8(FALSE)
	, m_checkbox_x9(FALSE)
	, m_checkbox_x10(FALSE)
	, m_checkbox_x11(FALSE)
	, m_checkbox_x12(FALSE)
	, m_checkbox_x13(FALSE)
	, m_checkbox_x14(FALSE)
	, m_checkbox_x15(FALSE)
	, m_checkbox_x16(FALSE)
	, m_checkbox_x17(FALSE)
	, m_checkbox_x18(FALSE)
	, m_checkbox_x19(FALSE)
	, m_edit_x0(_T("")) {
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

	DDX_Check(pDX, IDC_CHECK_X0, m_checkbox_x0);
	DDX_Check(pDX, IDC_CHECK_X1, m_checkbox_x1);
	DDX_Check(pDX, IDC_CHECK_X2, m_checkbox_x2);
	DDX_Check(pDX, IDC_CHECK_X3, m_checkbox_x3);
	DDX_Check(pDX, IDC_CHECK_X4, m_checkbox_x4);
	DDX_Check(pDX, IDC_CHECK_X5, m_checkbox_x5);
	DDX_Check(pDX, IDC_CHECK_X6, m_checkbox_x6);
	DDX_Check(pDX, IDC_CHECK_X7, m_checkbox_x7);
	DDX_Check(pDX, IDC_CHECK_X8, m_checkbox_x8);
	DDX_Check(pDX, IDC_CHECK_X9, m_checkbox_x9);
	DDX_Check(pDX, IDC_CHECK_X10, m_checkbox_x10);
	DDX_Check(pDX, IDC_CHECK_X11, m_checkbox_x11);
	DDX_Check(pDX, IDC_CHECK_X12, m_checkbox_x12);
	DDX_Check(pDX, IDC_CHECK_X13, m_checkbox_x13);
	DDX_Check(pDX, IDC_CHECK_X14, m_checkbox_x14);
	DDX_Check(pDX, IDC_CHECK_X15, m_checkbox_x15);
	DDX_Check(pDX, IDC_CHECK_X16, m_checkbox_x16);
	DDX_Check(pDX, IDC_CHECK_X17, m_checkbox_x17);
	DDX_Check(pDX, IDC_CHECK_X18, m_checkbox_x18);
	DDX_Check(pDX, IDC_CHECK_X19, m_checkbox_x19);
	DDX_Check(pDX, IDC_CHECK_X20, m_checkbox_x20);
	DDX_Check(pDX, IDC_CHECK_X21, m_checkbox_x21);
	DDX_Check(pDX, IDC_CHECK_X22, m_checkbox_x22);
	DDX_Check(pDX, IDC_CHECK_X23, m_checkbox_x23);
	DDX_Check(pDX, IDC_CHECK_X24, m_checkbox_x24);
	DDX_Check(pDX, IDC_CHECK_X25, m_checkbox_x25);
	DDX_Check(pDX, IDC_CHECK_X26, m_checkbox_x26);
	DDX_Check(pDX, IDC_CHECK_X27, m_checkbox_x27);
	DDX_Check(pDX, IDC_CHECK_X28, m_checkbox_x28);
	DDX_Check(pDX, IDC_CHECK_X29, m_checkbox_x29);
	DDX_Check(pDX, IDC_CHECK_X30, m_checkbox_x30);
	DDX_Check(pDX, IDC_CHECK_SP, m_checkbox_sp);
	DDX_Check(pDX, IDC_CHECK_PC, m_checkbox_pc);
	DDX_Check(pDX, IDC_CHECK_PSTATE, m_checkbox_pstate);
	DDX_Check(pDX, IDC_CHECK_ORIG_X0, m_checkbox_orig_x0);
	DDX_Check(pDX, IDC_CHECK_SYSCALLNO, m_checkbox_syscallno);

	DDX_Text(pDX, IDC_EDIT_X0, m_edit_x0);
	DDX_Text(pDX, IDC_EDIT_X1, m_edit_x1);
	DDX_Text(pDX, IDC_EDIT_X2, m_edit_x2);
	DDX_Text(pDX, IDC_EDIT_X3, m_edit_x3);
	DDX_Text(pDX, IDC_EDIT_X4, m_edit_x4);
	DDX_Text(pDX, IDC_EDIT_X5, m_edit_x5);
	DDX_Text(pDX, IDC_EDIT_X6, m_edit_x6);
	DDX_Text(pDX, IDC_EDIT_X7, m_edit_x7);
	DDX_Text(pDX, IDC_EDIT_X8, m_edit_x8);
	DDX_Text(pDX, IDC_EDIT_X9, m_edit_x9);
	DDX_Text(pDX, IDC_EDIT_X10, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X11, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X12, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X13, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X14, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X15, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X16, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X17, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X18, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X19, m_edit_x10);
	DDX_Text(pDX, IDC_EDIT_X20, m_edit_x20);
	DDX_Text(pDX, IDC_EDIT_X21, m_edit_x21);
	DDX_Text(pDX, IDC_EDIT_X22, m_edit_x22);
	DDX_Text(pDX, IDC_EDIT_X23, m_edit_x23);
	DDX_Text(pDX, IDC_EDIT_X24, m_edit_x24);
	DDX_Text(pDX, IDC_EDIT_X25, m_edit_x25);
	DDX_Text(pDX, IDC_EDIT_X26, m_edit_x26);
	DDX_Text(pDX, IDC_EDIT_X27, m_edit_x27);
	DDX_Text(pDX, IDC_EDIT_X28, m_edit_x28);
	DDX_Text(pDX, IDC_EDIT_X29, m_edit_x29);
	DDX_Text(pDX, IDC_EDIT_X30, m_edit_x30);
	DDX_Text(pDX, IDC_EDIT_SP, m_edit_sp);
	DDX_Text(pDX, IDC_EDIT_PC, m_edit_pc);
	DDX_Text(pDX, IDC_EDIT_PSTATE, m_edit_pstate);
	DDX_Text(pDX, IDC_EDIT_ORIG_X0, m_edit_orig_x0);
	DDX_Text(pDX, IDC_EDIT_SYSCALLNO, m_edit_syscallno);
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
	m_list_result.InsertColumn(0, L"PID", LVCFMT_LEFT, 60);//插入列
	m_list_result.InsertColumn(1, L"内存地址", LVCFMT_LEFT, 130);
	m_list_result.InsertColumn(2, L"类型", LVCFMT_LEFT, 50);
	m_list_result.InsertColumn(3, L"长度", LVCFMT_LEFT, 40);
	m_list_result.InsertColumn(4, L"线程", LVCFMT_LEFT, 40);
	m_list_result.InsertColumn(5, L"命中地址", LVCFMT_LEFT, 130);
	m_list_result.InsertColumn(6, L"信息", LVCFMT_LEFT, 350);


	m_edit_pid = L"0";
	m_edit_addr = L"0";
	m_checkbox_addr_hex = TRUE;
	m_edit_keep_time = L"250";
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



void CtestHwBpClientDlg::GetUserHitConditions(HIT_CONDITIONS & hitConditions) {
	if (m_checkbox_x0) {
		std::wstringstream ssConvert; ssConvert << m_edit_x0.GetString();
		hitConditions.enable_regs[0] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[0];
	}
	if (m_checkbox_x1) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x1.GetString();

		hitConditions.enable_regs[1] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[1];
	}
	if (m_checkbox_x1) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x1.GetString();

		hitConditions.enable_regs[1] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[1];
	}
	if (m_checkbox_x2) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x2.GetString();

		hitConditions.enable_regs[2] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[2];
	}
	if (m_checkbox_x3) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x3.GetString();

		hitConditions.enable_regs[3] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[3];
	}
	if (m_checkbox_x4) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x4.GetString();

		hitConditions.enable_regs[4] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[4];
	}
	if (m_checkbox_x5) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x4.GetString();

		hitConditions.enable_regs[5] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[5];
	}
	if (m_checkbox_x6) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x6.GetString();

		hitConditions.enable_regs[6] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[6];
	}
	if (m_checkbox_x7) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x7.GetString();

		hitConditions.enable_regs[7] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[7];
	}
	if (m_checkbox_x8) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x8.GetString();

		hitConditions.enable_regs[8] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[8];
	}
	if (m_checkbox_x9) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x9.GetString();

		hitConditions.enable_regs[9] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[9];
	}
	if (m_checkbox_x10) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x10.GetString();

		hitConditions.enable_regs[10] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[10];
	}
	if (m_checkbox_x11) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x11.GetString();

		hitConditions.enable_regs[11] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[11];
	}
	if (m_checkbox_x12) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x12.GetString();

		hitConditions.enable_regs[12] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[12];
	}
	if (m_checkbox_x13) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x13.GetString();

		hitConditions.enable_regs[13] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[13];
	}
	if (m_checkbox_x14) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x14.GetString();

		hitConditions.enable_regs[14] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[14];
	}
	if (m_checkbox_x15) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x15.GetString();

		hitConditions.enable_regs[15] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[15];
	}
	if (m_checkbox_x16) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x16.GetString();

		hitConditions.enable_regs[16] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[16];
	}
	if (m_checkbox_x17) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x17.GetString();

		hitConditions.enable_regs[17] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[17];
	}
	if (m_checkbox_x18) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x18.GetString();

		hitConditions.enable_regs[18] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[18];
	}
	if (m_checkbox_x19) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x19.GetString();

		hitConditions.enable_regs[19] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[19];
	}
	if (m_checkbox_x20) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x20.GetString();

		hitConditions.enable_regs[20] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[20];
	}
	if (m_checkbox_x21) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x21.GetString();

		hitConditions.enable_regs[21] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[21];
	}
	if (m_checkbox_x22) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x22.GetString();

		hitConditions.enable_regs[22] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[22];
	}
	if (m_checkbox_x23) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x23.GetString();

		hitConditions.enable_regs[23] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[23];
	}
	if (m_checkbox_x24) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x24.GetString();

		hitConditions.enable_regs[24] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[24];
	}
	if (m_checkbox_x25) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x25.GetString();

		hitConditions.enable_regs[25] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[25];
	}
	if (m_checkbox_x26) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x26.GetString();

		hitConditions.enable_regs[26] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[26];
	}
	if (m_checkbox_x27) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x27.GetString();

		hitConditions.enable_regs[27] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[27];
	}
	if (m_checkbox_x28) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x28.GetString();

		hitConditions.enable_regs[28] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[28];
	}
	if (m_checkbox_x29) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x29.GetString();

		hitConditions.enable_regs[29] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[29];
	}
	if (m_checkbox_x30) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_x30.GetString();

		hitConditions.enable_regs[30] = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.regs[30];
	}
	if (m_checkbox_sp) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_sp.GetString();

		hitConditions.enable_sp = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.sp;
	}
	if (m_checkbox_pc) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_pc.GetString();

		hitConditions.enable_pc = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.pc;
	}
	if (m_checkbox_pstate) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_pstate.GetString();

		hitConditions.enable_pstate = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.pstate;
	}
	if (m_checkbox_orig_x0) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_orig_x0.GetString();

		hitConditions.enable_orig_x0 = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.orig_x0;
	}
	if (m_checkbox_syscallno) {
		std::wstringstream ssConvert;
		ssConvert << m_edit_syscallno.GetString();

		hitConditions.enable_syscallno = '\x01';
		ssConvert >> std::hex >> hitConditions.regs.syscallno;
	}
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
	if (!g_NetworkManager.Reconnect()) {
		MessageBox(L"与服务器已失去连接");
		return;
	}

	//硬件断点类型
	uint32_t hwBpAddrLen = GetInputHwBpAddrLen();
	uint32_t hwBpAddrType = GetInputHwBpAddrType();
	uint32_t hwBpThreadType = m_radio_region_all_thread;
	uint32_t hwBpKeepTimeMs = GetInputHwBpKeepTimeMs();

	//硬件断点命中记录条件
	HIT_CONDITIONS hitConditions = { 0 };
	GetUserHitConditions(hitConditions);
	if (!g_NetworkManager.SetHwBpHitConditions(hitConditions)) {
		g_NetworkManager.Disconnect();
		MessageBox(L"设置硬件断点命中记录条件失败");
		return;
	}

	//开始安装硬件断点
	uint32_t allTaskCount;
	uint32_t insHwBpSuccessTaskCount;
	std::vector<USER_HIT_INFO> vHitResult;
	g_NetworkManager.AddProcessHwBp(
		pid,
		address,
		hwBpAddrLen,
		hwBpAddrType,
		hwBpThreadType,
		hwBpKeepTimeMs,
		allTaskCount,
		insHwBpSuccessTaskCount,
		vHitResult);


	//将命中结果显示到列表框
	int nIndex = m_list_result.GetItemCount();
	for (USER_HIT_INFO h : vHitResult) {

		m_list_result.InsertItem(nIndex, m_edit_pid);

		std::wstring showAddr = GetInputAddressString();
		transform(showAddr.begin(), showAddr.end(), showAddr.begin(), ::toupper);

		m_list_result.SetItemText(nIndex, 1, std::wstring(L"0x" + showAddr).c_str());
		m_list_result.SetItemText(nIndex, 2, m_radio_type_r == 0 ? L"R" : m_radio_type_r == 1 ? L"W" : m_radio_type_r == 2 ? L"RW" : L"X");
		m_list_result.SetItemText(nIndex, 3, m_radio_len_4 == 0 ? L"4" : L"8");
		m_list_result.SetItemText(nIndex, 4, m_radio_region_all_thread == 0 ? L"ALL" : m_radio_region_all_thread == 1 ? L"MAIN" : L"OTHER");

		std::wstringstream wssConvert;
		wssConvert << std::hex << h.hit_addr;
		showAddr = wssConvert.str();
		transform(showAddr.begin(), showAddr.end(), showAddr.begin(), ::toupper);

		m_list_result.SetItemText(nIndex, 5, std::wstring(L"0x" + showAddr).c_str());


		std::wstringstream ssInfoText;
		wchar_t info[4096] = { 0 };
		wsprintf(info, L"命中地址: 0x%llX 命中次数:%zu \r\n\r\n", h.hit_addr, h.hit_count);
		ssInfoText << info;

		for (int r = 0; r < 30; r += 5) {
			memset(info, 0, sizeof(info));
			wsprintf(info, L"X%-2d=%-12llX X%-2d=%-12llX X%-2d=%-12llX X%-2d=%-12llX X%-2d=%-12llX\r\n",
				r, h.regs.regs[r],
				r + 1, h.regs.regs[r + 1],
				r + 2, h.regs.regs[r + 2],
				r + 3, h.regs.regs[r + 3],
				r + 4, h.regs.regs[r + 4]);
			ssInfoText << info;
		}
		memset(info, 0, sizeof(info));
		wsprintf(info, L"\r\nLR= %-12llX SP= %-12llX PC= %-12llX\r\n\r\n",
			h.regs.regs[30],
			h.regs.sp,
			h.regs.pc);
		ssInfoText << info;

		memset(info, 0, sizeof(info));
		wsprintf(info, L"process status: %-12llX orig_x0: %-12llX\r\nsyscallno: %-12llX\r\n",
			h.regs.pstate,
			h.regs.orig_x0,
			h.regs.syscallno);

		ssInfoText << info;

		m_list_result.SetItemText(nIndex, 6, ssInfoText.str().c_str());

		//使刚刚插入的新项可见
		m_list_result.EnsureVisible(nIndex, FALSE);

		nIndex++;
	}
	if (allTaskCount == 0) {
		MessageBox(L"目标进程的线程数为0");
	} else if (allTaskCount != insHwBpSuccessTaskCount) {
		std::wstringstream wssBuf;
		wssBuf << L"发现有硬件断点安装失败的线程！目标进程的线程总数:" << allTaskCount << L",硬件断点安装成功线程数:" << insHwBpSuccessTaskCount;
		MessageBox(wssBuf.str().c_str());
	}
	g_NetworkManager.Disconnect();

}


void CtestHwBpClientDlg::OnDblclkListResult(NMHDR *pNMHDR, LRESULT *pResult) {
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	if (pNMItemActivate->iItem != -1) {
		//按下了列表
		CString title = m_list_result.GetItemText(pNMItemActivate->iItem, 6);
		int pos = title.Find(L"\r\n");
		if (pos != -1) {
			title = title.Left(pos);
		}
		if (title.GetLength() > 100) {
			title = title.Left(100);
		}

		CString text = m_list_result.GetItemText(pNMItemActivate->iItem, 6);
		text += "\r\n\r\n提示：\r\nR13寄存器（SP）：它是堆栈指针寄存器\r\nR14寄存器（LR）：它用于保存子程序的返回地址";

		//显示窗口
		std::thread td([](std::shared_ptr<std::wstring>title, std::shared_ptr<std::wstring>text)->void {
			CTextViewDlg dlg(title->c_str(), text->c_str());
			dlg.DoModal();
		}, std::make_shared<std::wstring>(title.GetString()), std::make_shared<std::wstring>(text.GetString()));
		td.detach();
	}
	*pResult = 0;
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

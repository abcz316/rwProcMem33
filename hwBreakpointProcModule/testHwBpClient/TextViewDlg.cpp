// TextViewDlg.cpp: 实现文件
//

#include "pch.h"
#include "testHwBpClient.h"
#include "TextViewDlg.h"
#include "afxdialogex.h"


// CTextViewDlg 对话框

IMPLEMENT_DYNAMIC(CTextViewDlg, CDialogEx)

CTextViewDlg::CTextViewDlg(CString title, CString text)
	: CDialogEx(IDD_TEXT_VIEW_DIALOG, GetDesktopWindow()) {
	m_title = title;
	m_edit_text = text;
}

CTextViewDlg::~CTextViewDlg() {
}

void CTextViewDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_TEXT, m_edit_text);
}


BEGIN_MESSAGE_MAP(CTextViewDlg, CDialogEx)
	ON_WM_CLOSE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
END_MESSAGE_MAP()


// CTextViewDlg 消息处理程序


BOOL CTextViewDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// TODO:  在此添加额外的初始化

	UpdateData(FALSE);

	m_font.CreateFont(16, 0, 0, 0, FW_EXTRABOLD, FALSE, FALSE, 0, ANSI_CHARSET, OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_MODERN,
		L"新宋体");

	GetDlgItem(IDC_EDIT_TEXT)->SetFont(&m_font);

	SetWindowTextW(m_title);
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


BOOL CTextViewDlg::PreTranslateMessage(MSG* pMsg) {
	// TODO: 在此添加专用代码和/或调用基类

	return CDialogEx::PreTranslateMessage(pMsg);
}


void CTextViewDlg::OnClose() {
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnClose();
}


void CTextViewDlg::OnDestroy() {
	CDialogEx::OnDestroy();

	// TODO: 在此处添加消息处理程序代码
}


void CTextViewDlg::OnSize(UINT nType, int cx, int cy) {
	CDialogEx::OnSize(nType, cx, cy);

	CWnd * edit = GetDlgItem(IDC_EDIT_TEXT);
	if (edit) {
		::SetWindowPos(edit->m_hWnd, NULL, 0, 0, cx, cy, SWP_NOMOVE);
	}
}

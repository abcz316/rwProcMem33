#pragma once


// CTextViewDlg 对话框

class CTextViewDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTextViewDlg)

public:
	CTextViewDlg(CString title,CString text);   // 标准构造函数
	virtual ~CTextViewDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TEXT_VIEW_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()

public:
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	afx_msg void OnClose();
	afx_msg void OnDestroy();
	afx_msg void OnSize(UINT nType, int cx, int cy); 

private:
	CString m_title;
	CString m_edit_text;
	CFont m_font;

};

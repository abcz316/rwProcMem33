#pragma once


// CNetworkDlg 对话框

class CNetworkDlg : public CDialogEx {
	DECLARE_DYNAMIC(CNetworkDlg)

public:
	CNetworkDlg(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~CNetworkDlg();

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NETWORK_DIALOG };
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
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedConnect();
	afx_msg void OnBnClickedCancel();
	afx_msg LRESULT OnAddFindServer(WPARAM wParam, LPARAM lParam);
	afx_msg void OnItemclickListFindServer(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnClickListFindServer(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnDblclkListFindServer(NMHDR *pNMHDR, LRESULT *pResult);
private:
	CListCtrl m_list_find_server;
	CString m_edit_ip;
	CString m_edit_port;
};

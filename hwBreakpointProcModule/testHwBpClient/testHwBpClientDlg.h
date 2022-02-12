
// testHwBpClientDlg.h: 头文件
//

#pragma once
#include "Global.h"

// CtestHwBpClientDlg 对话框
class CtestHwBpClientDlg : public CDialogEx {
	// 构造
public:
	CtestHwBpClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TESTHWBPCLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBnClickedButtonAddHwbp();
	afx_msg void OnDblclkListResult(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnRclickListResult(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnMenuitemCleanList();
	afx_msg void OnMenuitemDeleteSelectedCount();
	afx_msg void OnMenuitemDeleteOtherCount();
	afx_msg void OnMenuitemDeleteUpCount();
	afx_msg void OnMenuitemDeleteDownCount();
	DECLARE_MESSAGE_MAP()
private:
	CListCtrl m_list_result;
	CString m_edit_pid;
	CString m_edit_addr;
	CString m_edit_keep_time;
	BOOL m_checkbox_pid_hex;
	BOOL m_checkbox_addr_hex;
	int m_radio_type_r;
	int m_radio_len_4;
	int m_radio_region_all_thread;

	BOOL m_checkbox_x0;
	BOOL m_checkbox_x1;
	BOOL m_checkbox_x2;
	BOOL m_checkbox_x3;
	BOOL m_checkbox_x4;
	BOOL m_checkbox_x5;
	BOOL m_checkbox_x6;
	BOOL m_checkbox_x7;
	BOOL m_checkbox_x8;
	BOOL m_checkbox_x9;
	BOOL m_checkbox_x10;
	BOOL m_checkbox_x11;
	BOOL m_checkbox_x12;
	BOOL m_checkbox_x13;
	BOOL m_checkbox_x14;
	BOOL m_checkbox_x15;
	BOOL m_checkbox_x16;
	BOOL m_checkbox_x17;
	BOOL m_checkbox_x18;
	BOOL m_checkbox_x19;
	BOOL m_checkbox_x20;
	BOOL m_checkbox_x21;
	BOOL m_checkbox_x22;
	BOOL m_checkbox_x23;
	BOOL m_checkbox_x24;
	BOOL m_checkbox_x25;
	BOOL m_checkbox_x26;
	BOOL m_checkbox_x27;
	BOOL m_checkbox_x28;
	BOOL m_checkbox_x29;
	BOOL m_checkbox_x30;
	BOOL m_checkbox_sp;
	BOOL m_checkbox_pc;
	BOOL m_checkbox_pstate;
	BOOL m_checkbox_orig_x0;
	BOOL m_checkbox_syscallno;

	CString m_edit_x0;
	CString m_edit_x1;
	CString m_edit_x2;
	CString m_edit_x3;
	CString m_edit_x4;
	CString m_edit_x5;
	CString m_edit_x6;
	CString m_edit_x7;
	CString m_edit_x8;
	CString m_edit_x9;
	CString m_edit_x10;
	CString m_edit_x11;
	CString m_edit_x12;
	CString m_edit_x13;
	CString m_edit_x14;
	CString m_edit_x15;
	CString m_edit_x16;
	CString m_edit_x17;
	CString m_edit_x18;
	CString m_edit_x19;
	CString m_edit_x20;
	CString m_edit_x21;
	CString m_edit_x22;
	CString m_edit_x23;
	CString m_edit_x24;
	CString m_edit_x25;
	CString m_edit_x26;
	CString m_edit_x27;
	CString m_edit_x28;
	CString m_edit_x29;
	CString m_edit_x30;
	CString m_edit_sp;
	CString m_edit_pc;
	CString m_edit_pstate;
	CString m_edit_orig_x0;
	CString m_edit_syscallno;
private:
	void GetUserHitConditions(HIT_CONDITIONS & hitConditions);
	int GetInputPid();
	UINT64 GetInputAddress();
	std::wstring GetInputAddressString();
	uint32_t GetInputHwBpAddrLen();
	uint32_t GetInputHwBpAddrType();
	uint32_t GetInputHwBpKeepTimeMs();
};

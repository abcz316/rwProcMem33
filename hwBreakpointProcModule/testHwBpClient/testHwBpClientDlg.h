
// testHwBpClientDlg.h: 头文件
//

#pragma once
#include "Global.h"

enum {
	HIT_RESULT_COL_ID = 0,
	HIT_RESULT_COL_TIME,
	HIT_RESULT_COL_PID,
	HIT_RESULT_COL_TID,
	HIT_RESULT_COL_THREAD_RANGE,
	HIT_RESULT_COL_BP_ADDR,
	HIT_RESULT_COL_BP_TYPE,
	HIT_RESULT_COL_BP_LEN,
	HIT_RESULT_COL_HIT_ADDR,
	HIT_RESULT_COL_HIT_TOTAL_COUNT,
	HIT_RESULT_COL_HIT_INFO,
};

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
private:
	int GetInputPid();
	UINT64 GetInputAddress();
	std::wstring GetInputAddressString();
	uint32_t GetInputHwBpAddrLen();
	uint32_t GetInputHwBpAddrType();
	uint32_t GetInputHwBpKeepTimeMs();
	std::wstring TimestampToDatetime(uint64_t timestamp);
	BOOL PutTextToClipboard(LPCTSTR pTxtData);
public:
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMenuitemCopySelected();
	afx_msg void OnMenuitemCopyList();
};

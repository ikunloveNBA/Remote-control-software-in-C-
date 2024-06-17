
// RemoteClientDlg.h: 头文件
//

#pragma once
#include"ClientSocket.h"
#include "CStatusDlg.h"
#ifndef WM_SEND_PACK_ACK
#define WM_SEND_PACK_ACK (WM_USER+2)//发送数据包应答
#endif

// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
	// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

	// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持
public:
	void LoadFileInfo();//加载文件树的核心函数

private:
	bool m_isClosed;//放指远程监控时同时开启多个监控线程


private:
	void DealCommand(WORD nCmd,const std::string& strData, LPARAM  lParam);
	void InitUIData();
	void Str2Tree(const std::string& driver,CTreeCtrl&tree);
	void UpdateFileInfo(const FILEINFO& finfo, HTREEITEM hParent);
	void UpdateDownloadFile(const std::string& strData,FILE* pFile);
	void LoadFileCurrent();//加载当前文件（删除一个文件后使用此函数刷新）
	CString GetPath(HTREEITEM hTree);//获取文件路径
	void DeleteTreeChildrenItem(HTREEITEM hTree);//删除文件树的所有子节点，刷新用

	int SendCommandPacket(int nCmd, bool bAutoClose = true, BYTE* pData = NULL, size_t nLength = 0);//发送命令包
	// 实现
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//int count = 0;
	afx_msg void OnBnClickedBtnTest();//测试网络连接
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();//显示磁盘信息
	CTreeCtrl m_Tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);//左键双击显示文件树
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);//左键单击显示文件树
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);//右键单击文件显示menuBar
	afx_msg void OnDownloadFile();//下载文件
	afx_msg void OnDeleteFile();//删除文件
	afx_msg void OnRunFile();//运行文件
	afx_msg void OnBnClickedBtnStartWatch();//监控屏幕
	afx_msg void OnTimer(UINT_PTR nIDEvent);//定时器
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);//更新ip和端口
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);

	afx_msg void OnEnChangeEditPort();//更新ip和端口
};


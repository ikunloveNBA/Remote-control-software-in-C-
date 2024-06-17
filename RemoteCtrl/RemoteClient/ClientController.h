#pragma once
#include"ClientSocket.h"
#include"CWatchDialog.h"
#include"RemoteClientDlg.h"
#include"CStatusDlg.h"
#include"resource.h"
#include<map>
#include"ClientSocket.h"
#include"Tool.h"

//#define WM_SEND_DATA (WM_USER+2)//发送数据
#define WM_SHOW_STATUS (WM_USER+3)//展示状态
#define WMSHOW_WATCH (WM_USER+4)//远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000)//自定义消息处理

class CClientController
{
public:
	//获取全局唯一对象
	static CClientController* getInstance();
	//初始化操作
	int InitController();
	//启动
	int Invoke(CWnd*& pMainWnd);
	

	//更新网络服务器地址
	void UpdateAddress(int nIP,int nPort)
	{
		CClientSocket::getInstance()->UpdateAddress(nIP,nPort);
	}

	int DealCommand()
	{
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket()
	{
		CClientSocket::getInstance()->CloseSocket();
	}

	//1、查看磁盘分区
	//2、查看指定目录下的文件
	//3、打开文件
	//4、下载文件
	//9、删除文件
	//5、鼠标操作
	//6、发送屏幕内容
	//7、锁机
	//1981、测试连接
	//返回值：是状态，true是成功，false是失败

	bool SendCommandPacket(
		HWND hWnd,//数据包收到后，需要应答的窗口
		int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		WPARAM wParam=0
	);
	

	int GetImage(CImage& image)
	{
		CClientSocket* pClient = CClientSocket::getInstance();
		return CCTool::Bytes2Image(image, pClient->GetPacket().strData);
	}

	void DownLoadEnd();
	int DownFile(CString strPath);
	
	void StartWatchScreen();
protected:	
	void threadWatchScreen();
	static void threadWatchScreen(void*arg);
	CClientController() :m_statusDlg(&m_remoteDlg),m_watchDlg(&m_remoteDlg)
	{	
		m_isClosed = true;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_hThreadWatch = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}
	~CClientController() 
	{
		WaitForSingleObject(m_hThread, 100);
	}
	void threadFunc();
	static unsigned _stdcall threadEntry(void*arg);
	static void releaseInstance()
	{
		if (m_instance != NULL)
		{
			delete m_instance;
			m_instance = NULL;
		}
	}
	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	//LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);

	
private:
	typedef struct MsgInfo
	{
		MSG msg;//消息
		LRESULT result;//返回结果
		MsgInfo(MSG m)
		{
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m)
		{
			if (this != &m)
			{
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
		MsgInfo(const MsgInfo& m)
		{
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		

	}MSGINFO;
	typedef LRESULT(CClientController::*MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;//消息号和对应的消息函数
	static CClientController* m_instance;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	bool m_isClosed;//监视是否关闭
	CString m_strRemote;//下载文件的远程路径
	CString m_strLocal;//下载文件的本地保存路径

	unsigned m_nThreadID;

	//析构实例的辅助类
	class CHelper
	{
	public:
		CHelper()
		{

		}

		~CHelper()
		{
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;
	
};


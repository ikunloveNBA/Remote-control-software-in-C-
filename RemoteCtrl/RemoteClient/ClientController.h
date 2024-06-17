#pragma once
#include"ClientSocket.h"
#include"CWatchDialog.h"
#include"RemoteClientDlg.h"
#include"CStatusDlg.h"
#include"resource.h"
#include<map>
#include"ClientSocket.h"
#include"Tool.h"

//#define WM_SEND_DATA (WM_USER+2)//��������
#define WM_SHOW_STATUS (WM_USER+3)//չʾ״̬
#define WMSHOW_WATCH (WM_USER+4)//Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000)//�Զ�����Ϣ����

class CClientController
{
public:
	//��ȡȫ��Ψһ����
	static CClientController* getInstance();
	//��ʼ������
	int InitController();
	//����
	int Invoke(CWnd*& pMainWnd);
	

	//���������������ַ
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

	//1���鿴���̷���
	//2���鿴ָ��Ŀ¼�µ��ļ�
	//3�����ļ�
	//4�������ļ�
	//9��ɾ���ļ�
	//5��������
	//6��������Ļ����
	//7������
	//1981����������
	//����ֵ����״̬��true�ǳɹ���false��ʧ��

	bool SendCommandPacket(
		HWND hWnd,//���ݰ��յ�����ҪӦ��Ĵ���
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
		MSG msg;//��Ϣ
		LRESULT result;//���ؽ��
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
	static std::map<UINT, MSGFUNC> m_mapFunc;//��Ϣ�źͶ�Ӧ����Ϣ����
	static CClientController* m_instance;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;
	HANDLE m_hThreadWatch;
	bool m_isClosed;//�����Ƿ�ر�
	CString m_strRemote;//�����ļ���Զ��·��
	CString m_strLocal;//�����ļ��ı��ر���·��

	unsigned m_nThreadID;

	//����ʵ���ĸ�����
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


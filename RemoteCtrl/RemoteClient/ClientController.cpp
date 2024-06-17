#include "pch.h"
#include "ClientController.h"

CClientController* CClientController::m_instance=NULL;

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;//��Ϣ�źͶ�Ӧ����Ϣ����
CClientController::CHelper CClientController::m_helper;

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL)
	{
		m_instance = new CClientController();
		struct { UINT nMsg; MSGFUNC func; }MsgFuncs[] =
		{
			//{ WM_SEND_PACK,&CClientController::OnSendPack},
			//{WM_SEND_DATA,&CClientController::OnSendData},
			{WM_SHOW_STATUS,&CClientController::OnShowStatus},
			{WMSHOW_WATCH,&CClientController::OnShowWatcher},
			{-1,NULL}
		};
		for (int i = 0; MsgFuncs[i].func != NULL; i++)
		{
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(MsgFuncs[i].nMsg, MsgFuncs[i].func));
		}
	}																
	return m_instance;
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientController::threadEntry, this, 0, &m_nThreadID);
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;	
	return m_remoteDlg.DoModal();
}



bool CClientController::SendCommandPacket(HWND hWnd,int nCmd, bool bAutoClose, BYTE* pData, size_t nLength,WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret=pClient->SendPacket(hWnd,CPacket(nCmd, pData, nLength),bAutoClose,wParam);
	return ret;
	
}

void CClientController::DownLoadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();
	m_remoteDlg.MessageBox(_T("������ɣ�"), _T("���"));
}

int CClientController::DownFile (CString strPath)
{
	CFileDialog dlg(FALSE, "*", strPath,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	//�û�ѡ�����ļ�
	if (dlg.DoModal() == IDOK)
	{
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		FILE* pFile = fopen(m_strLocal, "wb+");
		if (pFile == NULL)
		{
			AfxMessageBox(_T("û��Ȩ�ޱ�����ļ��������ļ��޷�����������"));
			return -1;
		}
		SendCommandPacket(m_remoteDlg, 4, false, (BYTE*)(LPCSTR)m_strRemote, m_strRemote.GetLength(), (WPARAM)pFile);
		//m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		/*if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT)
		{
			return -1;
		}*/
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("��������ִ���У�"));
		m_statusDlg.ShowWindow(SW_SHOW);
		m_statusDlg.CenterWindow(&m_remoteDlg);
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = FALSE;
	m_hThreadWatch = (HANDLE)_beginthread(&CClientController::threadWatchScreen, 0, this);
	m_watchDlg.DoModal();
	m_isClosed = TRUE;
	WaitForSingleObject(m_hThreadWatch, 500);
}

void CClientController::threadWatchScreen()
{
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed)
	{
		//Sleep(10);
		if (m_watchDlg.isFull()==false)
		{
			if (GetTickCount64() - nTick < 200)
			{
				Sleep(200- DWORD(GetTickCount64() - nTick));//��200�뷢һ��
			}
			nTick = GetTickCount64();
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(),6,true,NULL,0);
			//TODO:�����Ϣ��Ӧ����WM_SEND_PACK_ACK	
			if (ret == 1)
			{
				
				TRACE("���óɹ�m_isFull\r\n");
			}
			else
			{
				TRACE("��ȡͼƬʧ�ܣ�ret=%d\r\n", ret);
			}
		}
		Sleep(1);
	}
}

void CClientController::threadWatchScreen(void*arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
}


void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_SEND_MESSAGE)
		{
			MSGINFO* pmsg = (MSGINFO*)msg.wParam; 
			HANDLE hEvent = (HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(pmsg->msg.message);
			if (it != m_mapFunc.end())
			{
				pmsg->result=(this->*it->second)(pmsg->msg.message, pmsg->msg.wParam, pmsg->msg.lParam);//��ȡ������Ϣ�����ķ���ֵ
				
			}
			else
			{
				pmsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else
		{
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end())
			{
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
		
	}
}

unsigned _stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();
	_endthreadex(0);
	return 0;
}

//LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	CPacket* pPacket = (CPacket*)wParam;
//	return pClient->Send(*pPacket);
//}

//LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
//{
//	CClientSocket* pClient = CClientSocket::getInstance();
//	char* pBuffer = (char*)wParam;
//	return pClient->Send(pBuffer, lParam);
//}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW); 
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();//��������
}

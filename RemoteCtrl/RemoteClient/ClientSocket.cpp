#include "pch.h"
#include "ClientSocket.h"
CClientSocket* CClientSocket::m_instance = nullptr;

//����ʵ���ĸ��������
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pserver = CClientSocket::getInstance();

CClientSocket::CClientSocket() :m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_bAutoClose(true), m_hThread(INVALID_HANDLE_VALUE)
{

	if (InitSockEnv() == FALSE)
	{
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ����������������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_eventInvoke = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	if (WaitForSingleObject(m_eventInvoke, 100) == WAIT_TIMEOUT)
	{
		TRACE("������Ϣ�����߳�����ʧ���ˣ�\r\n");
	}
	CloseHandle(m_eventInvoke);
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
	struct
	{
		UINT message;
		MSGFUNC func;
	}funcs[] =
	{
		{WM_SEND_PACK,&CClientSocket::SendPack },
		{0,NULL}
	};
	for (int i = 0; funcs[i].message != 0; i++)
	{
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].message, funcs[i].func)).second == false)
		{
			TRACE("����ʧ�ܣ���Ϣֵָ��%d ����ֵ��%08X ��ţ�%d\r\n", funcs[i].message, funcs[i].func, i);
		}
	}
}


CClientSocket::CClientSocket(const CClientSocket& ss)
{
	m_hThread = INVALID_HANDLE_VALUE;
	m_bAutoClose = ss.m_bAutoClose;
	this->m_sock = ss.m_sock;
	m_nIP = ss.m_nIP;
	m_nPort = ss.m_nPort;
	std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
	for (; it != ss.m_mapFunc.end(); it++)
	{
		m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
	}
}

bool CClientSocket::InitSocket()
{
	if (m_sock != INVALID_SOCKET)closesocket(m_sock);
	m_sock = socket(PF_INET, SOCK_STREAM, 0);
	if (m_sock == -1)
	{
		return false;
	}

	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(m_nIP);
	serv_adr.sin_port = htons(m_nPort);

	if (serv_adr.sin_addr.s_addr == INADDR_NONE)
	{
		AfxMessageBox("ָ����IP��ַ�������ڣ�");
		return false;
	}
	int ret = connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr));
	if (ret == -1)
	{
		AfxMessageBox("����ʧ��");
		TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());
	}
	return true;
}

std::string GetErrorInfo(int wasErrCode)
{
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wasErrCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}

unsigned CClientSocket::threadEntry(void* arg)
{
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}

//void CClientSocket::threadFunc()
//{
//	TRACE("����threadFunc\r\n");
//	std::string strBuffer;
//	strBuffer.resize(BUFFER_SIZE);
//	char* pBuffer = (char*)strBuffer.c_str();
//	int index = 0;
//	InitSocket();
//	while (m_sock != INVALID_SOCKET)
//	{
//		if (m_lstSend.size() > 0)
//		{
//			m_lock.lock();
//			CPacket& head = m_lstSend.front();
//			m_lock.unlock();
//			if (Send(head) == false)
//			{
//				TRACE("����ʧ�ܣ�\r\n");
//				continue;
//			}
//			TRACE("���ͳɹ���Send\r\n");
//			std::map<HANDLE, std::list<CPacket>&>::iterator it;
//			it = m_mapAck.find(head.hEvent);
//			if (it != m_mapAck.end())
//			{
//				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClosed.find(head.hEvent);
//				do {
//					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
//					TRACE("����length=%d\r\n", length);
//					if (length > 0 || (index > 0))
//					{
//						index += length;
//						size_t size = (size_t)index;
//						CPacket pack((BYTE*)pBuffer, size);
//
//						if (size > 0)
//						{
//							pack.hEvent = head.hEvent;
//							it->second.push_back(pack);
//							memmove(pBuffer, pBuffer + size, index - size);
//							index -= size;
//							if (it0->second)//��������������Ӱ��Ѿ����꣬�������¼�
//							{
//								SetEvent(head.hEvent);
//								break;
//							}
//						}
//					}
//					else if (length <= 0 && (index <= 0))
//					{
//						CloseSocket();
//						SetEvent(head.hEvent);//�ȵ��������ر�����֮����֪ͨ 
//						if (it0 != m_mapAutoClosed.end())
//						{
//
//						}
//						else
//						{
//							TRACE("�쳣�������û�ж�Ӧ��pair\r\n");
//						}
//						break;
//					}
//				} while (it0->second == false);
//			}
//			m_lock.lock();
//			m_lstSend.pop_front();
//			m_mapAutoClosed.erase(head.hEvent);
//			m_lock.unlock();
//			if (InitSocket() == false)
//				InitSocket();
//		}
//		else
//		{
//			Sleep(1);
//		}
//
//	}
//
//	CloseSocket();
//	TRACE("func�߳��˳�\r\n");
//}

bool CClientSocket::Send(const CPacket& pack)
{
	if (m_sock == -1)return false;
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0;
}

bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed, WPARAM wParam)
{

	UINT nMode = isAutoClosed ? CSM_AUTOCLOSE : 0;
	std::string strOut;
	pack.Data(strOut);
	PACKET_DATA* pData = new PACKET_DATA(strOut.c_str(), strOut.size(), nMode, wParam);
	
	bool ret=PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)pData, (LPARAM)hWnd);
	if (ret == false)
	{
		delete pData;
	}
	return ret;
}
/*
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed)
{
	TRACE("����SendPacket\r\n");
	if (m_sock == INVALID_SOCKET && m_hThread == INVALID_HANDLE_VALUE)
	{
		m_hThread = (HANDLE)_beginthread(&CClientSocket::threadEntry, 0, this);
	}

	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>(pack.hEvent, lstPacks));
	m_mapAutoClosed.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoClosed));
	//m_mapAck[pack.hEvent] = std::list<CPacket>();
	m_lstSend.push_back(pack);
	m_lock.unlock();
	WaitForSingleObject(pack.hEvent, INFINITE);

	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end())
	{
		TRACE("lst.size = %d\r\n", it->second.size());
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		//TRACE("�õ���\r\n");
		return true;
	}
	return false;
}
*/
void CClientSocket::threadFunc2()
{
	SetEvent(m_eventInvoke);
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end())
		{
			(this->*m_mapFunc[msg.message])(msg.message, msg.wParam, msg.lParam);
		}
	}
}

/*  wParam�ǰ������ݽṹ
	lParam��ĳ������������Ϣ���ڵľ������Ϊ���յ���Ϣ����һ��Ӧ��
*/
void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{//����һ����Ϣ�����ݽṹ �ص���Ϣ�����ݽṹ�����ݺ����ݳ��ȣ�ģʽ����HWND MESSAGE��

	PACKET_DATA data = *(PACKET_DATA*)wParam;//��ֹ�ڴ�й©��ʹ�þֲ���������
	delete (PACKET_DATA*)wParam;//ֱ���ͷţ��������ֲ������У���ֹ�ڴ�й©
	HWND hWnd = (HWND)lParam;
	size_t nTmp = data.strData.size();
	CPacket current((BYTE*)data.strData.c_str(), nTmp);
	if (InitSocket() == true)
	{
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0);
		if (ret > 0)
		{
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET)
			{
				int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
				if (length > 0 || (index > 0))//���ν��յ����ݻ��߻������ڻ���������
				{
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0)
					{
						::SendMessage(hWnd, WM_SNED_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);//���ܴ��ֲ�������ַ����ֹ������
						if (data.nMode & CSM_AUTOCLOSE)//ֻ����һ�����Ĵ������
						{
							CloseSocket();
							return;
						}
						memmove(pBuffer, pBuffer + nLen, index - nLen);
						index -= nLen;
					}
					

				}
				else//TODO:�Է��ر����׽��֣����������豸�쳣
				{
					CloseSocket();
					::SendMessage(hWnd, WM_SNED_PACK_ACK, (WPARAM)NULL, NULL);
				}
			}
			HWND hWnd = (HWND)lParam;
			::SendMessage(hWnd, WM_SNED_PACK_ACK, NULL, 1);
		}
		else
		{
			CloseSocket();
			::SendMessage(hWnd, WM_SNED_PACK_ACK, (WPARAM)new CPacket(current.sCmd,NULL,0), -1);
		}

	}
	else
	{
		::SendMessage(hWnd, WM_SNED_PACK_ACK, NULL, -2);
	}
}



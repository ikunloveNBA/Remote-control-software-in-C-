#include"pch.h"
#include"EServer.h"
#include "CTool.h"


//因为ACCEPT类和EClient相互依赖，得将类定义放到构造前面
EClient::EClient() :m_isBusy(false), m_flags(0),
m_overlapped(new ACCEPTOVERLAPPED()),
m_recv(new RECVOVERLAPPED()),
m_send(new SENDOVERLAPPED()),
m_vecSend( this, (SENDCALLBACK)&EClient::SendData)
{
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
		memset(&m_laddr, 0, sizeof(m_laddr));
	memset(&m_raddr, 0, sizeof(m_raddr));

}

void EClient::SetOverlapped(PCLIENT& ptr)
{
	m_overlapped->m_client = ptr.get();
	m_recv->m_client = ptr.get();
	m_send->m_client = ptr.get();
}

EClient::operator LPOVERLAPPED()
{
	return &m_overlapped->m_overlapped;
}

LPWSABUF EClient::RecvWSABuffer()
{

	return &m_recv->m_wsabuffer;

}

LPWSABUF EClient::SendWSABuffer()
{
	return &m_send->m_wsabuffer;
}


int EServer::threadIocp()
{
	{
		DWORD transferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* lpOverlapped = NULL;
		if (GetQueuedCompletionStatus((HANDLE)m_hIOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))
		{
			if (CompletionKey != 0)
			{
				EOverLapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, EOverLapped, m_overlapped);
				pOverlapped->m_server = this;
				switch (pOverlapped->m_operator)
				{
				case EAccept:
				{
					ACCEPTOVERLAPPED* pOver = (ACCEPTOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case ERecv:
				{
					RECVOVERLAPPED* pOver = (RECVOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case ESend:
				{
					SENDOVERLAPPED* pOver = (SENDOVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				case EError:
				{
					ERROROVERLAPPED* pOver = (ERROROVERLAPPED*)pOverlapped;
					m_pool.DispatchWorker(pOver->m_worker);
				}
				break;
				}
			}
			else
			{
				return -1;
			}

		}
		return 0;
	}
}

bool EServer::StartService()
{
	CreateSocket();
	if (bind(m_sock, (const sockaddr*)&m_addr, sizeof(m_addr)) == -1)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	if (listen(m_sock, 3) == -1)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	if (m_hIOCP == NULL)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.Invoke();
	m_pool.DispatchWorker(::ThreadWorker(this, (FUNCTYPE)&EServer::threadIocp));
	if (!NewAccept())return false;
	return true;
}

void EServer::BindNewSocket(SOCKET s)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}

int EClient::Recv()
{
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0)
	{
		return -1;
	}
	m_used += (size_t)ret;
	CCTool::Dump((BYTE*)m_buffer.data(), ret);
	return ret;
}

int EClient::Send(void* buffer, size_t nSize)
{
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data))
	{
		return 0;
	}
	return -1;
}

int EClient::SendData(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0)
	{
		int ret = WSASend(m_sock, SendWSABuffer(), 1, &m_received, m_flags, &m_send->m_overlapped, NULL);
		if (ret != 0 && (WSAGetLastError() != WSA_IO_PENDING))
		{
			CCTool::ShowError();
			return -1;
		}
	}
	return 0;
}

EServer::~EServer() 
{
	closesocket(m_sock);
	std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();
	for (;it!=m_client.end();it++)
	{
		it->second.reset();
	}
	m_client.clear();
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup(); 
}

LPWSAOVERLAPPED EClient:: RecvOverlapped()
{ return &m_recv->m_overlapped; }

LPWSAOVERLAPPED EClient::SendOverlapped()
{ return &m_send->m_overlapped; }
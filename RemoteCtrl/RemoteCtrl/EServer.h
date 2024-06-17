#pragma once
#include "CCThread.h"
#include<map>
#include<memory>
#include"CQueue.h"
#include<MSWSock.h>
#include<list>
#include "Command.h"



enum EOperator
{
	ENone,
	EAccept,
	ERecv,
	ESend,
	EError
};
class EClient;
typedef std::shared_ptr<EClient> PCLIENT;
class EServer;

class EOverLapped
{
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;//操作
	std::vector<char> m_buffer;//缓冲区
	ThreadWorker m_worker;//处理函数
	EServer* m_server;//服务器对象
	EClient* m_client;//对应的客户端
	WSABUF m_wsabuffer;//收数据的buffer
	virtual ~EOverLapped()
	{
		m_buffer.clear();
	}
};

template<EOperator>class AcceptOverlapped;
typedef AcceptOverlapped<EAccept> ACCEPTOVERLAPPED;
template<EOperator>class RecvOverlapped;
typedef RecvOverlapped<ERecv>RECVOVERLAPPED;
template<EOperator>class SendOverlapped;
typedef SendOverlapped<ESend>SENDOVERLAPPED;


class EClient:public ThreadFuncBase
{
public:
	EClient();

	~EClient()
	{
		m_buffer.clear();
		closesocket(m_sock);
		m_recv.reset();
		m_send.reset();
		m_overlapped.reset();

	}

	void SetOverlapped(PCLIENT& ptr);
	operator SOCKET()
	{
		return m_sock;
	}
	operator PVOID()
	{
		return &m_buffer[0];
	}
	operator LPOVERLAPPED();
	operator LPDWORD()
	{
		return &m_received; 
	}
	LPDWORD getReceived()
	{	
		return &m_received;
	}
	LPWSABUF RecvWSABuffer();
	LPWSAOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPWSAOVERLAPPED SendOverlapped();
	DWORD& flags() { return m_flags; }
	sockaddr_in* GetLocalAddr() { return &m_laddr; }
	sockaddr_in* GetRemoteAddr() { return &m_raddr; }
	size_t GetBufferSize()const { return m_buffer.size(); }
	int Recv();
	int Send(void* buffer, size_t nSize);
	int SendData(std::vector<char>& data);
	std::shared_ptr<RECVOVERLAPPED>& getRecv() { return m_recv; }
	std::list<CPacket> sendPackets;
	std::vector<char>m_buffer;
	SOCKET m_sock;
private:


	DWORD m_received;
	DWORD m_flags;
	std::shared_ptr<ACCEPTOVERLAPPED> m_overlapped;//accept的重叠体
	std::shared_ptr<RECVOVERLAPPED>m_recv;
	std::shared_ptr<SENDOVERLAPPED>m_send;

	size_t m_used;//已经使用的缓冲区大小
	sockaddr_in m_laddr;//本地地址
	sockaddr_in m_raddr;//远程地址
	bool m_isBusy;
	ESendQueue<std::vector<char>>m_vecSend;//发送数据队列
};


//Accept的重叠结构
template<EOperator>
class AcceptOverlapped :public EOverLapped, ThreadFuncBase
{
public:
	AcceptOverlapped();
	
	int AcceptWorker();
};



template<EOperator>
class RecvOverlapped :public EOverLapped, ThreadFuncBase
{
public:
	RecvOverlapped();
	int RecvWorker()
	{
		CCommand m_ccmd;
		int ret = m_client->Recv();
		CPacket pack((BYTE*)m_client->m_buffer.data(), (size_t&)ret);
		m_ccmd.ExcuteCommand(pack.sCmd, m_client->sendPackets, pack);
		while (m_client->sendPackets.size() > 0)
		{
			m_client->SendWSABuffer()->buf = const_cast<char*>(m_client->sendPackets.front().Data());
			WSASend(m_client->m_sock, m_client->SendWSABuffer(), 1, *m_client, m_client->flags(), m_client->SendOverlapped(), NULL);
		}


		return -1;
	}

};

template<EOperator>
class SendOverlapped :public EOverLapped, ThreadFuncBase
{
public:
	SendOverlapped();
	int SendWorker()
	{

		return -1;
	}

};


template<EOperator>
class ErrorOverlapped :public EOverLapped, ThreadFuncBase
{
public:
	ErrorOverlapped() :m_operator(EError), m_worker(this, &ErrorOverlapped::ErrorWorker)
	{
		memset(&m_overlapped, 0, sizeof(m_overlapped));
		m_buffer.resize(1024);
	}
	int ErrorWorker()
	{
		return -1; 
	}

};
typedef ErrorOverlapped<EError>ERROROVERLAPPED;



class EServer :
	public ThreadFuncBase
{
public:
	EServer(const std::string& ip = "0.0.0.0", short port = 9527) :m_pool(10)
	{
		m_hIOCP = INVALID_HANDLE_VALUE;
		memset(&m_addr, 0, sizeof(m_addr));
		//m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
		m_addr.sin_port = htons(port);
	}
	~EServer();

	bool StartService();
	void BindNewSocket(SOCKET s);
	
	bool NewAccept()
	{
		PCLIENT pClient(new EClient());
		pClient->SetOverlapped(pClient);
		m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));//重载了EClient的强制转换运算符
		if (FALSE == AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient))
		{
			if (WSAGetLastError() != WSA_IO_PENDING)
			{
				closesocket(m_sock);
				m_sock = INVALID_SOCKET;
				m_hIOCP = INVALID_HANDLE_VALUE;
				return false;
			}
			
		
		}
		return true;
	}
private:
	void CreateSocket()
	{
		WSAData WSAData;
		WSAStartup(MAKEWORD(2, 2), &WSAData);
		m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		int opt = 1;
		opt = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));//实现端口复用
	}
	
	int threadIocp();
	
private:
	CCThreadPool m_pool;//线程池
	HANDLE m_hIOCP;//完成端口
	SOCKET m_sock;//负责监听的套接字
	sockaddr_in m_addr;//负责监听的套接字的地址
	std::map<SOCKET, std::shared_ptr<EClient>>m_client;//客户端map
};

template<EOperator op>
AcceptOverlapped<op>::AcceptOverlapped()
{
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	m_operator = EAccept;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024);
	m_server = NULL;
}

template<EOperator op>
int AcceptOverlapped<op>::AcceptWorker() {

	INT lLength = 0, rLength = 0;
	size_t n = m_client->GetBufferSize();
	TRACE("%d\r\n", n);
	if (m_client->GetBufferSize() > 0)
	{
		sockaddr* plocal=NULL, * premote=NULL;
		GetAcceptExSockaddrs(*m_client, 0,
			sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&plocal, &lLength,//本地地址
			(sockaddr**)&premote, &rLength//远程地址
		);//接收到一个，马上投递下一个
		memcpy(m_client->GetLocalAddr(), plocal, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), premote, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client);
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client,& m_client->flags(), &(m_client->getRecv()->m_overlapped), NULL);

		if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING))
		{
			//报错
		}

		if (m_server->NewAccept() == false)
		{
			return -2;
		}
	}

	return -1;
}

template<EOperator eo>
inline SendOverlapped<eo>::SendOverlapped()
{
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<eo>::SendWorker);
	m_operator = eo;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

template<EOperator op>
inline RecvOverlapped<op>::RecvOverlapped() 
{
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	m_operator = op;
	memset(&m_overlapped, 0, sizeof(m_overlapped));
	m_buffer.resize(1024 * 256);
}

typedef ESendQueue<std::vector<char>>::ECALLBACK SENDCALLBACK;
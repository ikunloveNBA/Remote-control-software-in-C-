#include "pch.h"
#include "iocpServer.h"
#include<algorithm>
#include"CTool.h"
_PER_IO_CONTEXT::_PER_IO_CONTEXT()
{
	m_operator = IO_EVENT::ENone;
	m_overLapped = { 0 };
	m_buffer.resize(MAX_BUFFER_SIZE);
	m_wsaBuf.buf = &m_buffer[0];
	m_wsaBuf.len = m_buffer.size();
	//char* m_buffer = new char[MAX_BUFFER_SIZE];

}

void _PER_IO_CONTEXT::setIoClient(std::shared_ptr<IOClient> ioClient)
{
	m_IoClient = ioClient;
}

_PER_IO_CONTEXT::~_PER_IO_CONTEXT()
{
	/*if (m_buffer != NULL)
	{
		char* tmp = m_buffer;
		m_buffer=NULL;
		delete tmp;
	}*/
}

_PER_SOCKET_CONTEXT::_PER_SOCKET_CONTEXT()
{
	this->m_clientSock = INVALID_SOCKET;
	memset(&this->m_clientAddr, 0, sizeof(m_clientAddr));
	m_acceptContext = std::make_shared<IO_CONTEXT_ACCEPT>();
	m_acceptContext->m_operator = IO_EVENT::EAccept;
	m_recvContext = std::make_shared<IO_CONTEXT_RECV>();
	m_recvContext->m_operator = IO_EVENT::ERecv;
	m_sendContext = std::make_shared<IO_CONTEXT_SEND>();
	m_sendContext->m_operator = IO_EVENT::ESend;

}

bool _PER_SOCKET_CONTEXT::InitSocket()
{
	m_clientSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (m_clientSock == INVALID_SOCKET)
	{
		return false;
	}
	int opt = 1;
	opt = setsockopt(m_clientSock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
	return true;
}




iocpServer::iocpServer(unsigned port, std::string ip) :_PER_SOCKET_CONTEXT()
{
	memset(&m_servaddr, 0, sizeof(m_servaddr));
	m_sock = INVALID_SOCKET;
	m_IOCP = INVALID_HANDLE_VALUE;

	m_servaddr.sin_addr.S_un.S_addr = inet_addr(ip.c_str());
	m_servaddr.sin_family = AF_INET;
	m_servaddr.sin_port = htons(port);

}

void iocpServer::createSocket()
{
	WSAData data;
	int ret = WSAStartup(MAKEWORD(2, 2), &data);
	if (ret != 0)
	{
		TRACE("create wsa error!\r\n");
		return;
	}
	m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	opt = setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

bool iocpServer::Start()
{
	if (-1 == bind(m_sock, (sockaddr*)&m_servaddr, sizeof(m_servaddr)))
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	if (-1 == listen(m_sock, 5))
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_IOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if (m_IOCP == INVALID_HANDLE_VALUE)
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	HANDLE _IOCP = CreateIoCompletionPort((HANDLE)m_sock, m_IOCP, (ULONG_PTR)this, 4);
	TRACE("_IOCP=%d\r\n", _IOCP);
	TRACE("m_IOCP=%d\r\n", m_IOCP);
	NewThreadPool::instance().Commit([this] {this->threadIocp(); });
	return true;
}

int iocpServer::threadIocp()
{
	DWORD transferred = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	while (GetQueuedCompletionStatus((HANDLE)m_IOCP, &transferred, &CompletionKey, &lpOverlapped, INFINITE))//在这里get不到
	{
		TRACE("m_IOCP=%d\r\n", m_IOCP);
		if (CompletionKey != 0)
		{
			PER_IO_CONTEXT* _io_context = CONTAINING_RECORD(lpOverlapped, PER_IO_CONTEXT, m_overLapped);
			switch (_io_context->m_operator)
			{
			case IO_EVENT::EAccept:
			{
				IO_CONTEXT_ACCEPT* _io_accept_context = (IO_CONTEXT_ACCEPT*)_io_context;
				NewThreadPool::instance().Commit([this, _io_accept_context]() {_io_accept_context->AcceptFunc(this, m_IOCP); });
			}
			break;
			case IO_EVENT::ERecv:
			{
				IO_CONTEXT_RECV* _io_recv_context = (IO_CONTEXT_RECV*)_io_context;
				NewThreadPool::instance().Commit([this, _io_recv_context]() {_io_recv_context->RecvFunc(this); });
			}
			break;
			case IO_EVENT::ESend:
			{
				IO_CONTEXT_SEND* _io_send_context = (IO_CONTEXT_SEND*)_io_context;
				NewThreadPool::instance().Commit([this, _io_send_context]() {_io_send_context->SendFunc(this); });
			}
			break;
			default:
				break;
			}
		}
	}
	return 0;
}

bool iocpServer::NewAccept()
{
	LPFN_ACCEPTEX   m_lpfnAcceptEx;//AcceptEx函数指针
	GUID GuidAcceptEx = WSAID_ACCEPTEX;//缓冲区标识符
	DWORD dwBytes = 0;
	WSAIoctl(m_sock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidAcceptEx, sizeof(GuidAcceptEx),
		&m_lpfnAcceptEx, sizeof(m_lpfnAcceptEx),
		&dwBytes,
		NULL,
		NULL);
	//创建智能指针的客户端只要是为了在多线程传递时避免客户端内存被析构，客户端只要没有下线就一直存在
	std::shared_ptr<IOClient> io_client = std::make_shared<IOClient>();
	io_client->InitSocket();
	m_clients.push_back(io_client);
	io_client->m_acceptContext->setIoClient(io_client);
	if (FALSE == AcceptEx(m_sock, io_client->m_clientSock, (PVOID)&io_client->m_acceptContext->m_buffer[0],//这个AcceptEx
		0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		&io_client->m_acceptContext->m_dwRecievedBytes, &io_client->m_acceptContext->m_overLapped))
	{
		DWORD dwError = GetLastError();
		TRACE("error=%d\r\n", dwError);
		if (GetLastError() != WSA_IO_PENDING)
		{
			closesocket(m_sock);
			CloseHandle(m_IOCP);
			m_sock = INVALID_SOCKET;
			m_IOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

int IO_CONTEXT_ACCEPT::AcceptFunc(iocpServer* server, HANDLE hIOCP)
{

	auto _Ptr = this->m_IoClient.lock();

	LPFN_GETACCEPTEXSOCKADDRS lpfnGetAcceptExSockAddrs;
	GUID GuidGetAccpetExSockAddrs;
	DWORD dwBytes = 0;
	WSAIoctl(server->m_sock,
		SIO_GET_EXTENSION_FUNCTION_POINTER,
		&GuidGetAccpetExSockAddrs,
		sizeof(GuidGetAccpetExSockAddrs),
		&lpfnGetAcceptExSockAddrs,
		sizeof(lpfnGetAcceptExSockAddrs),
		&dwBytes,
		NULL,
		NULL
	);
	sockaddr* plocal = NULL, * pRemote = NULL;
	INT lLength = 0, rLength = 0;
	GetAcceptExSockaddrs((PVOID)&_Ptr->m_acceptContext->m_buffer[0], 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
		(sockaddr**)&plocal, &lLength, (sockaddr**)&pRemote, &rLength);
	//拿到客户端套接字
	server->BindNewAccept(_Ptr->m_clientSock);
	_Ptr->m_recvContext->setIoClient(_Ptr);
	
	std::unique_lock lock(this->m_mt_recive);
	_Ptr->m_recvContext->m_cv_recive.wait(lock, [_Ptr]() {return _Ptr->m_recvContext->m_reciveIsReady == true; });
	int ret = WSARecv(_Ptr->m_clientSock, &_Ptr->m_recvContext->m_wsaBuf, 1,
		&_Ptr->m_recvContext->m_dwBytesRecieved, &m_flags, &_Ptr->m_recvContext->m_overLapped, NULL);
	_Ptr->m_recvContext->m_reciveIsReady == false;
	if (ret == SOCKET_ERROR && (WSAGetLastError() != WSA_IO_PENDING))
	{
		int ret2 = WSAGetLastError();
		return -3;
	}
	if (server->NewAccept() == false)
	{
		return -2;
	}

	return 1;
}


void iocpServer::RunServer()
{
	this->createSocket();
	TRACE("m_sock=%d\r\n", m_sock);
	if (!Start())
	{
		return;
	}
	TRACE("m_iocp=%d\r\n", m_IOCP);
	if (!NewAccept())
	{
		return;
	}
	//RecvPacket();
	TRACE("%s\r\n", this->m_clients[0]->m_recvContext->m_buffer);
	getchar();

}


void iocpServer::SendPakcet()
{

}

bool IO_CONTEXT_RECV::RecvFunc(iocpServer* server)
{
	if (this->m_dwBytesRecieved > 0)
	{
		size_t len = m_dwBytesRecieved;
		TRACE("data=%s\r\n", this->m_buffer);
		auto _Ptr = this->m_IoClient.lock();
		_Ptr->m_pack=CPacket ((BYTE*)&m_buffer[0], len);

		if (len > 0)
		{
			this->m_buffer.clear();
			this->m_buffer.resize(MAX_BUFFER_SIZE);
			this->m_reciveIsReady = true;
			this->m_cv_recive.notify_one();
			CCommand::RunCommand(&server->m_cmd,_Ptr->m_pack.sCmd, _Ptr->m_listPack, _Ptr->m_pack);
			while (_Ptr->m_listPack.size() > 0)
			{
				if (_Ptr->m_sendContext->sendIsReady==true)
				{
					_Ptr->m_sendContext->setIoClient(_Ptr);
					CCTool::Dump((BYTE*)_Ptr->m_listPack.front().Data(), _Ptr->m_listPack.front().Size());
					//memcpy(&_Ptr->m_sendContext->m_buffer[0], _Ptr->m_listPack.front().Data(), _Ptr->m_listPack.front().Size());
					_Ptr->m_sendContext->m_wsaBuf.buf = const_cast<char*>(_Ptr->m_listPack.front().Data());
					_Ptr->m_sendContext->m_wsaBuf.len = _Ptr->m_listPack.front().Size();
					CCTool::Dump((BYTE*)_Ptr->m_sendContext->m_wsaBuf.buf, _Ptr->m_sendContext->m_wsaBuf.len);
					WSASend(_Ptr->m_clientSock, &_Ptr->m_sendContext->m_wsaBuf, 1,
						&_Ptr->m_sendContext->m_dwBytesSend, 0, &_Ptr->m_sendContext->m_overLapped, NULL);
					_Ptr->m_sendContext->sendIsReady = false;
					
				}
				
			}

			return true;
		}
		return false;
	}
	return false;
}

bool IO_CONTEXT_SEND::SendFunc(iocpServer* server)
{
	auto _Ptr = this->m_IoClient.lock();
	_Ptr->m_listPack.pop_front();
	/*this->m_buffer.clear();
	this->m_buffer.resize(MAX_BUFFER_SIZE);*/
	this->sendIsReady = true;
	if (_Ptr->m_listPack.empty())
	{
		server->closeClient(_Ptr->m_clientSock);
	}
	
	return true;
}

void iocpServer::BindNewAccept(SOCKET s)
{
	CreateIoCompletionPort((HANDLE)s, m_IOCP, (ULONG_PTR)this, 0);
}

void  iocpServer::closeClient(SOCKET s)
{	
	for (auto it = this->m_clients.begin(); it != m_clients.end(); it++)
	{
		std::shared_ptr<IOClient> _Ptr = *it;
		if (_Ptr->m_clientSock== s)
		{
			closesocket(_Ptr->m_clientSock);
			_Ptr->m_clientSock = INVALID_SOCKET;
			m_clients.erase(it);
			break;
		}
	}
}

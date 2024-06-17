#pragma once
#include<MSWSock.h>
#include"NewThreadPool.h"
#include"Packet.h"
#include"Command.h"
#include"CQueue.h"
#include<map>
#include<list>
#include<atomic>
#include<mutex>
#define MAX_BUFFER_SIZE 4096

enum class IO_EVENT
{
	ENone=0,
	EAccept,
	ESend,
	ERecv
};

class IOClient;
//io上下文
typedef struct _PER_IO_CONTEXT
{

	OVERLAPPED				m_overLapped;//重叠结构
	WSABUF					m_wsaBuf;//wsa缓冲区
	std::vector<char>		m_buffer;//数据缓冲区，wsa对应
	IO_EVENT				m_operator;//io操作符
	std::weak_ptr<IOClient> m_IoClient;//客户端结构体

	void setIoClient(std::shared_ptr<IOClient> ioClient);

	_PER_IO_CONTEXT();
	~_PER_IO_CONTEXT();
}PER_IO_CONTEXT, *PPER_IO_CONTEXT;

class IO_CONTEXT_ACCEPT;
class IO_CONTEXT_RECV;
class IO_CONTEXT_SEND;

//socket上下文  作为competionKey来标识每一个io操作是哪个套接字发出的
typedef struct _PER_SOCKET_CONTEXT
{
	_PER_SOCKET_CONTEXT();
	bool InitSocket();
	
	SOCKET											m_clientSock;
	sockaddr_in										m_clientAddr;//保存的客户端套接字的ip和端口
	std::shared_ptr<IO_CONTEXT_ACCEPT>				m_acceptContext;//监听io
	std::shared_ptr<IO_CONTEXT_RECV>				m_recvContext;//接收io
	std::shared_ptr<IO_CONTEXT_SEND>				m_sendContext;//发送io

}PER_SOCKET_CONTEXT,*PPER_SOCKET_CONTEXT;

class iocpServer;

class IO_CONTEXT_ACCEPT:public _PER_IO_CONTEXT
{
public://初始化
	IO_CONTEXT_ACCEPT() :_PER_IO_CONTEXT() ,m_dwRecievedBytes(0) ,m_flags(0) {}
	int AcceptFunc(iocpServer*server,HANDLE hIOCP);
	DWORD m_dwRecievedBytes;//每次accept到的字节数，其实没用，但是得有
	DWORD m_flags;
	
	std::mutex m_mt_recive;
};

class IO_CONTEXT_RECV :public _PER_IO_CONTEXT
{
public:
	IO_CONTEXT_RECV() :_PER_IO_CONTEXT() ,m_dwBytesRecieved(0), m_flags(0) , m_reciveIsReady(true) {}
	bool RecvFunc(iocpServer* server);
	DWORD m_dwBytesRecieved;
	DWORD m_flags;
	std::atomic_bool m_reciveIsReady;		//每次接受是否成功
	std::condition_variable m_cv_recive;	//条件变量
};

class IO_CONTEXT_SEND :public _PER_IO_CONTEXT
{
public:
	IO_CONTEXT_SEND() :_PER_IO_CONTEXT(), m_dwBytesSend(0),sendIsReady(true) {}
	bool SendFunc(iocpServer* server);
	DWORD m_dwBytesSend = 0;
	std::atomic_bool sendIsReady;		//每一个包数据是否发送完毕
	
};

class IOClient:public _PER_SOCKET_CONTEXT
{
public:
	IOClient() :_PER_SOCKET_CONTEXT() {}
	std::list<CPacket> m_listPack;		//要发送的数据包
	CPacket m_pack;						//接收的命令包
};





class iocpServer:public _PER_SOCKET_CONTEXT
{
public:
	friend class IO_CONTEXT_ACCEPT;
	friend class IO_CONTEXT_RECV;
	friend class IO_CONTEXT_SEND;

	iocpServer(unsigned port = 9527, std::string ip = "127.0.0.1") ;
	iocpServer(const iocpServer&) = delete;//只创建一个服务端，禁用拷贝
	iocpServer& operator=(const iocpServer&) = delete;
	void createSocket();
	bool Start();
	int threadIocp();
	bool NewAccept();
	void RunServer();
	void SendPakcet();
	void BindNewAccept(SOCKET s);
	void closeClient(SOCKET s);
	~iocpServer(){}
	HANDLE getIocp()
	{
		return m_IOCP;
	}
private:
	SOCKET m_sock;//服务端套接字
	HANDLE m_IOCP;//完成端口
	sockaddr_in m_servaddr;//服务端ip和端口
	std::vector<std::shared_ptr<IOClient>>m_clients;//在线的客户端
	CCommand m_cmd;
};

 //NewThreadPool* iocpServer::m_pool =&( NewThreadPool::instance());
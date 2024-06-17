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
//io������
typedef struct _PER_IO_CONTEXT
{

	OVERLAPPED				m_overLapped;//�ص��ṹ
	WSABUF					m_wsaBuf;//wsa������
	std::vector<char>		m_buffer;//���ݻ�������wsa��Ӧ
	IO_EVENT				m_operator;//io������
	std::weak_ptr<IOClient> m_IoClient;//�ͻ��˽ṹ��

	void setIoClient(std::shared_ptr<IOClient> ioClient);

	_PER_IO_CONTEXT();
	~_PER_IO_CONTEXT();
}PER_IO_CONTEXT, *PPER_IO_CONTEXT;

class IO_CONTEXT_ACCEPT;
class IO_CONTEXT_RECV;
class IO_CONTEXT_SEND;

//socket������  ��ΪcompetionKey����ʶÿһ��io�������ĸ��׽��ַ�����
typedef struct _PER_SOCKET_CONTEXT
{
	_PER_SOCKET_CONTEXT();
	bool InitSocket();
	
	SOCKET											m_clientSock;
	sockaddr_in										m_clientAddr;//����Ŀͻ����׽��ֵ�ip�Ͷ˿�
	std::shared_ptr<IO_CONTEXT_ACCEPT>				m_acceptContext;//����io
	std::shared_ptr<IO_CONTEXT_RECV>				m_recvContext;//����io
	std::shared_ptr<IO_CONTEXT_SEND>				m_sendContext;//����io

}PER_SOCKET_CONTEXT,*PPER_SOCKET_CONTEXT;

class iocpServer;

class IO_CONTEXT_ACCEPT:public _PER_IO_CONTEXT
{
public://��ʼ��
	IO_CONTEXT_ACCEPT() :_PER_IO_CONTEXT() ,m_dwRecievedBytes(0) ,m_flags(0) {}
	int AcceptFunc(iocpServer*server,HANDLE hIOCP);
	DWORD m_dwRecievedBytes;//ÿ��accept�����ֽ�������ʵû�ã����ǵ���
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
	std::atomic_bool m_reciveIsReady;		//ÿ�ν����Ƿ�ɹ�
	std::condition_variable m_cv_recive;	//��������
};

class IO_CONTEXT_SEND :public _PER_IO_CONTEXT
{
public:
	IO_CONTEXT_SEND() :_PER_IO_CONTEXT(), m_dwBytesSend(0),sendIsReady(true) {}
	bool SendFunc(iocpServer* server);
	DWORD m_dwBytesSend = 0;
	std::atomic_bool sendIsReady;		//ÿһ���������Ƿ������
	
};

class IOClient:public _PER_SOCKET_CONTEXT
{
public:
	IOClient() :_PER_SOCKET_CONTEXT() {}
	std::list<CPacket> m_listPack;		//Ҫ���͵����ݰ�
	CPacket m_pack;						//���յ������
};





class iocpServer:public _PER_SOCKET_CONTEXT
{
public:
	friend class IO_CONTEXT_ACCEPT;
	friend class IO_CONTEXT_RECV;
	friend class IO_CONTEXT_SEND;

	iocpServer(unsigned port = 9527, std::string ip = "127.0.0.1") ;
	iocpServer(const iocpServer&) = delete;//ֻ����һ������ˣ����ÿ���
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
	SOCKET m_sock;//������׽���
	HANDLE m_IOCP;//��ɶ˿�
	sockaddr_in m_servaddr;//�����ip�Ͷ˿�
	std::vector<std::shared_ptr<IOClient>>m_clients;//���ߵĿͻ���
	CCommand m_cmd;
};

 //NewThreadPool* iocpServer::m_pool =&( NewThreadPool::instance());
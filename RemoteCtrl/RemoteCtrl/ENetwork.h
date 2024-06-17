#pragma once
#include"ESocket.h"
#include"CCThread.h"
class ENetwork
{

};

typedef int (*AcceptFunc)(void* arg,ESocket& client);
typedef int (*RecvFunc)(void* arg, const EBuffer& buffer);
typedef int (*SendFunc)(void* arg);
typedef int (*RecvFromFunc)(void* arg, const EBuffer& buffer,ESockaddrIn&addr);
typedef int (*SendToFunc)(void* arg);

class EServerParameter
{
public:

	EServerParameter(
		const std::string& ip = "0.0.0.0",
		short port = 9527,
		ETYPE type = ETYPE::ETypeTCP,
		AcceptFunc acceptf = NULL,
		RecvFunc recvf = NULL,
		SendFunc snedf = NULL,
		RecvFromFunc recvfromf = NULL,
		SendToFunc sendtof = NULL
	) {}
	//输入
	EServerParameter& operator<<(AcceptFunc func);
	EServerParameter& operator<<(RecvFunc func);
	EServerParameter& operator<<(SendFunc func);
	EServerParameter& operator<<(RecvFromFunc func);
	//EServerParameter& operator<<(SendToFunc func);
	EServerParameter& operator<<(const std::string& ip);
	EServerParameter& operator<<(short port);
	EServerParameter& operator<<(ETYPE type);
	//输出
	EServerParameter& operator>>(AcceptFunc& func);
	EServerParameter& operator>>(RecvFunc& func);
	EServerParameter& operator>>(SendFunc& func);
	EServerParameter& operator>>(RecvFromFunc func);
	EServerParameter& operator>>(SendToFunc func);
	EServerParameter& operator>>(std::string& ip);
	EServerParameter& operator>>(short& port);
	EServerParameter& operator>>(ETYPE& type);
	//复制构造函数，等于号重载 用于同类型的赋值
	EServerParameter(const EServerParameter& param);
	EServerParameter& operator=(const EServerParameter& param);
	std::string m_ip;
	short m_port;
	ETYPE m_type;
	AcceptFunc m_accept;
	RecvFunc m_recv;
	SendFunc m_send;
	RecvFromFunc m_recvfrom;
	SendToFunc m_sendto;
};

//class ECServer:public ThreadFuncBase
//{
//public:
//	ECServer(const EServerParameter&param);
//	~ECServer();
//	int Invoke(void* arg);
//	int Send(ESOCKET& client, const EBuffer& buffer);
//	int Sendto( ESockaddrIn& addr,const EBuffer&buffer);
//	int Stop();
//private:
//	int threadFunc();
//private:
//	EServerParameter m_params;
//	CCThread m_thread;
//};

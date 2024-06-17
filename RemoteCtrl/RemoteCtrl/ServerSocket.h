#pragma once
#include"pch.h"
#include"framework.h"
#include<list>
#include"Packet.h"
#include<MSWSock.h>


//单例模式
//因为windows中套接字库的建立和释放在整个代码中只需要实现一次即可，所以可以使用单例模式来创建一个套接字库的类来管理
//在单例模式中，用户不可使用该类随意创建对象，所以需要将该类中的构造函数，拷贝构造以及赋值运算发等都设为私有函数
//并且需要使用静态方法来获取全局唯一的静态变量

typedef void (*SOCKET_CALLBACK)(void*,int,std::list<CPacket>&,CPacket&);

class CServerSocket
{
public:
	//获取单例对象
	static CServerSocket* getInstance()//静态函数没有this指针，所以无法访问静态变量
	{
		if (m_instance == nullptr)
		{
			m_instance = new CServerSocket();
		}
		 return m_instance;
	}

	

	int Run(SOCKET_CALLBACK callback, void* arg,short port=1527)
	{
		//1、进度的可控性 2、对接的方便性 3、可行性评估，提早暴露风险 
			// TODO: socket,bind,listen,accept,read,write,close
			//套接字初始化
		bool ret = InitSocket(port);
		if (ret == false)
		{
			return -1;
		}
		std::list<CPacket>lstPackets;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true)
		{
			if (AcceptClient() == false)
			{
				if (count >= 3)
				{
					return -2;
				}
				count++;
			}
			int ret = DealCommand();
			if (ret > 0)
			{
				m_callback(m_arg, ret,lstPackets,m_packet);
				while(lstPackets.size() > 0)
				{
					Send(lstPackets.front());
					lstPackets.pop_front();
				}
			}
			CloseClient();

		}
	
		
	}

protected:
	bool InitSocket(short port = 1527)
	{
		if (m_sock == -1)
		{
			return false;
		}

		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);

		//绑定
		if (::bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr))==-1)
		{
			return false;
		}

		if (listen(m_sock, 1) == -1)
		{
			return false;

		}
		return true;
	}
	bool AcceptClient()
	{
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client = accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1)
		{
			return false;
		}
		return true;
	}
#define BUFFER_SIZE 4096
	int DealCommand()
	{
		if (m_client == -1)return -1;
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		if (buffer == NULL)
		{
			TRACE("内存不足");
		}
		size_t index = 0;
		while (true)
		{
			size_t len = recv(m_client, buffer + index, BUFFER_SIZE - index, 0);
			TRACE("Client recv Len %d\r\n", len);
			if (len <= 0)
			{
				delete[] buffer;
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				delete[] buffer;
				return m_packet.sCmd;	
			}
		}
		delete[] buffer;
		return -1;
	}

	bool Send(const char* pData, int nSize)
	{
		if (m_client == -1)return false;
		return send(m_client, pData, nSize, 0) > 0;
	}
	bool Send(CPacket& pack)
	{
		if (m_client == -1)return false;
		return send(m_client, pack.Data(), pack.Size(), 0);
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

	void CloseClient()
	{
		if (m_client != INVALID_SOCKET)
		{
			closesocket(m_client);
			m_client = INVALID_SOCKET;


		}
	}
private:
	SOCKET_CALLBACK m_callback;
	void* m_arg;
	SOCKET m_client;
	SOCKET m_sock;
	CPacket m_packet;

	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss)
	{
		this->m_sock = ss.m_sock;
		this->m_client = ss.m_client;
	}
	CServerSocket()
	{
		this->m_sock = INVALID_SOCKET;
		this->m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE)
		{
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置！"), _T("初始化错误！"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);

	}
	~CServerSocket()
	{
		closesocket(m_sock);
		WSACleanup();
	}
	//初始化套接字库
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}
	//释放单例对象（对单例对象的控制一定要使用静态方法）
	static void releaseInstance()
	{
		if (m_instance != nullptr)
		{
			//为什么不直接delete，因为直接将指针置空汇编指令很少，执行时间非常短，而如果直接delete，那么之间可能会比较长，当代码量多的时候
			//可能别人调用m_instance的时候它正在析构，还没有被置空，所以可以先置空再delete
			CServerSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}

	}


	//设置为指针是为了动态创建实例，
	static CServerSocket* m_instance;

	//析构实例的辅助类
	class CHelper
	{
	public:
		CHelper()
		{
			CServerSocket::getInstance();
		}

		~CHelper()
		{
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};



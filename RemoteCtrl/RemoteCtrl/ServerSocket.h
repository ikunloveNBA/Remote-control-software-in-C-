#pragma once
#include"pch.h"
#include"framework.h"
#include<list>
#include"Packet.h"
#include<MSWSock.h>


//����ģʽ
//��Ϊwindows���׽��ֿ�Ľ������ͷ�������������ֻ��Ҫʵ��һ�μ��ɣ����Կ���ʹ�õ���ģʽ������һ���׽��ֿ����������
//�ڵ���ģʽ�У��û�����ʹ�ø������ⴴ������������Ҫ�������еĹ��캯�������������Լ���ֵ���㷢�ȶ���Ϊ˽�к���
//������Ҫʹ�þ�̬��������ȡȫ��Ψһ�ľ�̬����

typedef void (*SOCKET_CALLBACK)(void*,int,std::list<CPacket>&,CPacket&);

class CServerSocket
{
public:
	//��ȡ��������
	static CServerSocket* getInstance()//��̬����û��thisָ�룬�����޷����ʾ�̬����
	{
		if (m_instance == nullptr)
		{
			m_instance = new CServerSocket();
		}
		 return m_instance;
	}

	

	int Run(SOCKET_CALLBACK callback, void* arg,short port=1527)
	{
		//1�����ȵĿɿ��� 2���Խӵķ����� 3�����������������籩¶���� 
			// TODO: socket,bind,listen,accept,read,write,close
			//�׽��ֳ�ʼ��
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

		//��
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
			TRACE("�ڴ治��");
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
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ����������������ã�"), _T("��ʼ������"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);

	}
	~CServerSocket()
	{
		closesocket(m_sock);
		WSACleanup();
	}
	//��ʼ���׽��ֿ�
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}
	//�ͷŵ������󣨶Ե�������Ŀ���һ��Ҫʹ�þ�̬������
	static void releaseInstance()
	{
		if (m_instance != nullptr)
		{
			//Ϊʲô��ֱ��delete����Ϊֱ�ӽ�ָ���ÿջ��ָ����٣�ִ��ʱ��ǳ��̣������ֱ��delete����ô֮����ܻ�Ƚϳ��������������ʱ��
			//���ܱ��˵���m_instance��ʱ����������������û�б��ÿգ����Կ������ÿ���delete
			CServerSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}

	}


	//����Ϊָ����Ϊ�˶�̬����ʵ����
	static CServerSocket* m_instance;

	//����ʵ���ĸ�����
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



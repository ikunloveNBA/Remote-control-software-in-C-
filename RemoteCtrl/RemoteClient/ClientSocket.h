#pragma once
#include"pch.h"
#include"framework.h"
#include<string>
#include <vector>
#include<list>
#include<map>
#include<mutex>
#define WM_SEND_PACK (WM_USER + 1)//���Ͱ�����
#define WM_SNED_PACK_ACK (WM_USER+2)//���Ͱ�����Ӧ��
//���ݰ���
#pragma pack(push)
#pragma pack(1)
class CPacket
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {};
	CPacket(const CPacket& pack)
	{
		this->sHead = pack.sHead;
		this->nLength = pack.nLength;
		this->sCmd = pack.sCmd;
		this->strData = pack.strData;
		this->sSum = pack.sSum;
	}
	CPacket(WORD nCmd, const BYTE* pdata, size_t nSize)
	{
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0)
		{
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pdata, nSize);
		}
		else
		{
			strData.clear();
		}
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const BYTE* pData, size_t& nSize)
	{
		/*
		* �������
		* pData ���ݰ�ָ��
		* nSize����Σ����ݰ��ĳ���
		* nSize�����Σ�ʵ��ʹ�����ݰ��ĳ���
		*/

		size_t i = 0;
		for (i; i < nSize; i++)//���̶�λ
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)//���������+��������+У��� > �ð������򷵻�
		{
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize)//��û����ȫ�յ����ͷ��أ�����ʧ��
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4)//��ȡ���е�����
		{
			strData.resize(nLength - 4);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;
		for (size_t j = 0; j < strData.size(); j++)
		{
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum)
		{
			nSize = i;
			return;
		}
		nSize = 0;
	}
	CPacket& operator=(const CPacket& pack)
	{
		if (this != &pack)
		{
			this->sHead = pack.sHead;
			this->nLength = pack.nLength;
			this->sCmd = pack.sCmd;
			this->strData = pack.strData;
			this->sSum = pack.sSum;
		}
		return *this;
	}
	int Size()//�����ݵĴ�С
	{
		return nLength + 6;
	}
	const char* Data(std::string& strOut)const
	{
		strOut.resize(nLength + 6);
		BYTE* pdata = (BYTE*)strOut.c_str();
		*(WORD*)pdata = sHead; pdata += 2;
		*(DWORD*)pdata = nLength; pdata += 4;
		*(WORD*)pdata = sCmd; pdata += 2;
		memcpy(pdata, (void*)strData.c_str(), strData.size()); pdata += strData.size();
		*(WORD*)pdata = sSum;
		return strOut.c_str();
	}
	~CPacket() {};
public:
	WORD sHead;//�̶�λFE FF
	DWORD nLength;//�����ȣ��ӿ������ʼ������У�������
	WORD sCmd;//��������
	std::string strData;//������
	WORD sSum;//��У��
};

#pragma pack(pop)

typedef struct MouseEvent
{
	MouseEvent()
	{
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//������ƶ���˫��
	WORD nButton;//������Ҽ����м�
	POINT ptXY;//����
}MOUSEEV, * PMOUSEEV;

typedef struct  file_info
{
	file_info()
	{
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid;//�Ƿ���Ч
	BOOL IsDirectory;//�Ƿ���Ŀ¼
	BOOL HasNext;//�Ƿ��к���
	char szFileName[256];//�ļ���Ŀ¼��
}FILEINFO, * PFILEINFO;

//��Ϣ���ݽṹ
typedef struct PacketData
{
	std::string strData;
	UINT nMode;
	WPARAM wParam;
	PacketData(const char* pData, size_t nLen, UINT mode,WPARAM nParam=0)
	{
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMode = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& data)
	{
		strData = data.strData;
		nMode = data.nMode;
		wParam = data.wParam;
	}
	PacketData& operator=(const PacketData& data)
	{
		if (this != &data)
		{
			strData = data.strData;
			nMode = data.nMode;
			wParam = data.wParam;

		}
		return *this;
	}
}PACKET_DATA;

enum
{
	CSM_AUTOCLOSE=1,//CSM=Client Socket Mode �Զ��ر�ģʽ
};


//��������
extern std::string GetErrorInfo(int wasErrCode);


//����ģʽ
//��Ϊwindows���׽��ֿ�Ľ������ͷ�������������ֻ��Ҫʵ��һ�μ��ɣ����Կ���ʹ�õ���ģʽ������һ���׽��ֿ����������
//�ڵ���ģʽ�У��û�����ʹ�ø������ⴴ������������Ҫ�������еĹ��캯�������������Լ���ֵ���㷢�ȶ���Ϊ˽�к���
//������Ҫʹ�þ�̬��������ȡȫ��Ψһ�ľ�̬����

class CClientSocket
{
public:
	//��ȡ��������
	static CClientSocket* getInstance()//��̬����û��thisָ�룬�����޷����ʾ�̬����
	{
		if (m_instance == nullptr)
		{
			m_instance = new CClientSocket();
		}
	     return m_instance;
	}

	bool InitSocket();
	

#define BUFFER_SIZE 4096000
	int DealCommand()
	{
		if (m_sock == -1)return -1;
		char* buffer = m_buffer.data();

		static size_t index = 0;
		while (true)
		{
			size_t len = recv(m_sock, buffer + index, BUFFER_SIZE - index, 0);
			if (((int)len <= 0)&&((int)index<=0))
			{
				return -1;
			}
			index += len;
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0)
			{
				memmove(buffer, buffer + len, BUFFER_SIZE - len);
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}

	
	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPacks, bool isAutoClosed = true);
	bool SendPacket(HWND hWnd, const CPacket& pack, bool isAutoClosed = true,WPARAM wParam=0);

	//��ȡ�ļ�·��
	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	//��ȡ����¼�
	bool GetMouseEvent(MOUSEEV& mouse)
	{
		if (m_packet.sCmd == 5)
		{
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(MOUSEEV));
			return true;
		}
		return false;
	}

	CPacket& GetPacket()
	{
		return m_packet;
	}

	void CloseSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}

	void UpdateAddress(int nIP, int nPort)
	{
		if (m_nIP != nIP || m_nPort != nPort)
		{
			m_nIP = nIP;
			m_nPort = nPort;
		}
		
	}
private:
	HANDLE m_eventInvoke;//�����¼�
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC>m_mapFunc;
	HANDLE m_hThread;
	std::mutex m_lock;
	bool m_bAutoClose;
	std::list<CPacket>m_lstSend;//�ͻ��˽�Ҫ���͵����ݶ��У�һ��������Ӧ�Ķ�����ݣ�
	std::map<HANDLE, std::list<CPacket>&>m_mapAck;//�ͻ���Ҫ�������˷���������map
	std::map < HANDLE, bool >m_mapAutoClosed;
	int m_nIP;//��ַ
	int m_nPort;//�˿�
	std::vector<char>m_buffer;//������
	SOCKET m_sock;//�׽���
	CPacket m_packet;//���ݰ�

	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss);
	CClientSocket();

	~CClientSocket()
	{
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	}
	static unsigned _stdcall threadEntry(void* arg);
	//void threadFunc();
	void threadFunc2();

	//��ʼ���׽��ֿ�
	BOOL InitSockEnv()
	{
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0)
		{
			return FALSE;
		}
		return TRUE;
	}
	bool Send(const char* pData, int nSize)
	{
		if (m_sock == -1)return false;
		return send(m_sock, pData, nSize, 0) > 0;
	}
	bool Send(const CPacket& pack);
	void SendPack(UINT nMsg, WPARAM wParam/*��������ֵ*/, LPARAM lParam/*�������ĳ���*/);
	//�ͷŵ������󣨶Ե�������Ŀ���һ��Ҫʹ�þ�̬������
	static void releaseInstance()
	{
		if (m_instance != nullptr)
		{
			CClientSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}
	} 


	//����Ϊָ����Ϊ�˶�̬����ʵ����
	static CClientSocket* m_instance;

	//����ʵ���ĸ�����
	class CHelper
	{
	public:
		CHelper()
		{
			CClientSocket::getInstance();
		}

		~CHelper()
		{
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;
};



#pragma once
#include"pch.h"
#include"framework.h"
#include<string>
#include <vector>
#include<list>
#include<map>
#include<mutex>
#define WM_SEND_PACK (WM_USER + 1)//发送包数据
#define WM_SNED_PACK_ACK (WM_USER+2)//发送包数据应答
//数据包类
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
		* 解包函数
		* pData 数据包指针
		* nSize（入参）数据包的长度
		* nSize（出参）实际使用数据包的长度
		*/

		size_t i = 0;
		for (i; i < nSize; i++)//检查固定位
		{
			if (*(WORD*)(pData + i) == 0xFEFF)
			{
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize)//如果包长度+控制命令+校验和 > 该包长度则返回
		{
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize)//包没有完全收到，就返回，解析失败
		{
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		if (nLength > 4)//读取包中的数据
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
	int Size()//包数据的大小
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
	WORD sHead;//固定位FE FF
	DWORD nLength;//包长度（从控制命令开始，到和校验结束）
	WORD sCmd;//控制命令
	std::string strData;//包数据
	WORD sSum;//和校验
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
	WORD nAction;//点击、移动、双击
	WORD nButton;//左键，右键，中键
	POINT ptXY;//坐标
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
	BOOL IsInvalid;//是否有效
	BOOL IsDirectory;//是否是目录
	BOOL HasNext;//是否还有后续
	char szFileName[256];//文件或目录名
}FILEINFO, * PFILEINFO;

//消息数据结构
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
	CSM_AUTOCLOSE=1,//CSM=Client Socket Mode 自动关闭模式
};


//错误处理函数
extern std::string GetErrorInfo(int wasErrCode);


//单例模式
//因为windows中套接字库的建立和释放在整个代码中只需要实现一次即可，所以可以使用单例模式来创建一个套接字库的类来管理
//在单例模式中，用户不可使用该类随意创建对象，所以需要将该类中的构造函数，拷贝构造以及赋值运算发等都设为私有函数
//并且需要使用静态方法来获取全局唯一的静态变量

class CClientSocket
{
public:
	//获取单例对象
	static CClientSocket* getInstance()//静态函数没有this指针，所以无法访问静态变量
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

	//获取文件路径
	bool GetFilePath(std::string& strPath)
	{
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4))
		{
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	//获取鼠标事件
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
	HANDLE m_eventInvoke;//启动事件
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC>m_mapFunc;
	HANDLE m_hThread;
	std::mutex m_lock;
	bool m_bAutoClose;
	std::list<CPacket>m_lstSend;//客户端将要发送的数据队列（一个操作对应的多个数据）
	std::map<HANDLE, std::list<CPacket>&>m_mapAck;//客户端要处理服务端发来的数据map
	std::map < HANDLE, bool >m_mapAutoClosed;
	int m_nIP;//地址
	int m_nPort;//端口
	std::vector<char>m_buffer;//缓冲区
	SOCKET m_sock;//套接字
	CPacket m_packet;//数据包

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

	//初始化套接字库
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
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);
	//释放单例对象（对单例对象的控制一定要使用静态方法）
	static void releaseInstance()
	{
		if (m_instance != nullptr)
		{
			CClientSocket* tmp = m_instance;
			m_instance = nullptr;
			delete tmp;
		}
	} 


	//设置为指针是为了动态创建实例，
	static CClientSocket* m_instance;

	//析构实例的辅助类
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



#pragma once
#include"pch.h"
#include"framework.h"

#pragma pack(push)
#pragma pack(1)

//数据包类
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
			strData.resize(nLength - 2 - 2);
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
	const char* Data()
	{
		strOut.resize(nLength + 6);
		BYTE* pdata = (BYTE*)strOut.c_str();
		*(WORD*)pdata = sHead; pdata += 2;
		*(DWORD*)pdata = nLength; pdata += 4;
		*(WORD*)pdata = sCmd; pdata += 2;
		memcpy(pdata, strData.c_str(), strData.size()); pdata += strData.size();
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
	std::string strOut;//整个包的数据
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
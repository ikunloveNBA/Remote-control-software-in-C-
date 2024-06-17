#pragma once
#include"pch.h"
#include"framework.h"

#pragma pack(push)
#pragma pack(1)

//���ݰ���
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
	int Size()//�����ݵĴ�С
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
	WORD sHead;//�̶�λFE FF
	DWORD nLength;//�����ȣ��ӿ������ʼ������У�������
	WORD sCmd;//��������
	std::string strData;//������
	WORD sSum;//��У��
	std::string strOut;//������������
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
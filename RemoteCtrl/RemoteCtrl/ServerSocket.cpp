#include "pch.h"
#include "ServerSocket.h"

//���о�̬������Ҫ��ʾ��ʼ����������ʹ�ù��캯����ʼ��
CServerSocket* CServerSocket::m_instance = nullptr;

//����ʵ���ĸ��������
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();

#include "pch.h"
#include "ServerSocket.h"

//类中静态属性需要显示初始化，不可以使用构造函数初始化
CServerSocket* CServerSocket::m_instance = nullptr;

//析构实例的辅助类对象
CServerSocket::CHelper CServerSocket::m_helper;

CServerSocket* pserver = CServerSocket::getInstance();

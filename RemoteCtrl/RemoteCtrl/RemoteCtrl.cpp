// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include"ServerSocket.h"
#include"CTool.h"
#include<direct.h>
#include<atlimage.h>
#include<conio.h>
#include"Command.h"
#include"CQueue.h"
#include"EServer.h"
#include"ENetwork.h"
#include"iocpServer.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;

using namespace std;

void ChooseAutoInvoke()
{
	/*改bug的思路
	* 0 观察现象
	* 1 先确定范围
	* 2 分析错误的可能性
	* 3 调试或打日志
	* 4 处理错误
	* 5 验证/长时间验证/多次验证/多条件验证
	*/
}


void Dump(BYTE* pData, size_t nSize)
{
	std::string strOut;
	for (size_t i = 0; i < nSize; i++)
	{
		char buf[8] = "";
		if (i > 0 && (i % 16 == 0))strOut += "\n";
		snprintf(buf, sizeof(buf), "%02X ", pData[i] & 0xFF);
		strOut += buf;
	}
	strOut += "\n";
	OutputDebugStringA(strOut.c_str());
}




void getAdimin()
{
	if (CCTool::IsAdmin())
	{
		OutputDebugString(_T("current is run as administrator!\r\n"));
		//MessageBox(NULL, _T("管理员"), _T("用户状态"), 0);
	}
	else
	{
		OutputDebugString(_T("current is run as normal user!\r\n"));
		//MessageBox(NULL, _T("普通用户"), _T("用户状态"), 0);

		CCTool::RunAsAdmin();
		return;
	}
}

bool Init()
{

	HMODULE hModule = ::GetModuleHandle(nullptr);
	if (hModule == nullptr)
	{
		wprintf(L"错误: GetModuleHandle 失败\n");
		return false;
	}
	if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
	{
		// TODO: 在此处为应用程序的行为编写代码。
		wprintf(L"错误: MFC 初始化失败\n");
		return false;
	}
	return true;
}


#define IOCP_LIST_EMPTY 0
#define IOCP_LIST_PUSH 1
#define IOCP_LIST_POP 2

enum 
{
	IocpListEmpty,
	IocpListPush,
	IocpListPop,
};

typedef struct IocpParam
{

	int nOperator;//操作
	std::string strData;//数据
	_beginthread_proc_type cbFunc;//回调
	IocpParam(int op, const char* Data, _beginthread_proc_type cb=NULL)
	{
		nOperator = op;
		strData = Data;
		cbFunc = cb;
	}
	~IocpParam()
	{
		nOperator = -1;
	}
}IOCP_PARAM;


void threadQueryEntry(HANDLE hIOCP)
{
	std::list<std::string>lstString;
	DWORD dwTransferred = 0;
	ULONG_PTR CompletionKey=0;
	OVERLAPPED* pOverlapped = NULL;
	while (GetQueuedCompletionStatus(hIOCP, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE))
	{
		if ((dwTransferred == 0) || (CompletionKey == NULL))
		{
			printf("thread is prepare to exit!\r\n");//收到空消息然后结束
			break;
		}
		IOCP_PARAM* pParam = (IOCP_PARAM*)CompletionKey;
		if (pParam->nOperator == IocpListPush)
		{
			lstString.push_back(pParam->strData);
		}
		else if(pParam->nOperator==IocpListPop)
		{
			std::string* pStr = NULL;
			if(lstString.size()>0)
			{
				pStr=new std::string(lstString.front());
				lstString.pop_front();
			}
			if (pParam->cbFunc == NULL)
			{
				pParam->cbFunc(pStr);
			}
		}
		else if (pParam->nOperator == IocpListEmpty)
		{
			lstString.clear();
		}
		delete pParam;
	}
	_endthread();//在这里结束线程会导致线程函数并没有执行完，从而导致局部对象无法调用析构函数而导致内存泄露，
				//正确做法应该在线程函数里再调用一个真正的执行函数
}

void func(void* arg)
{
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL)
	{
		printf("pop from list:%s\r\n", pstr->c_str());
		delete pstr;
	}
	else
	{
		printf("list is empty\r\n");
	}
	
}

void test()
{
	CQueue<std::string>lstString;
	ULONGLONG tick0 = GetTickCount64(), tick1 = GetTickCount64(), total = GetTickCount64();
	while(GetTickCount64()-total<=1000)
	{
		
		if (GetTickCount64() - tick0 > 13)
		{
			lstString.PushBack("hello ,world!");
			tick0 = GetTickCount64();
		
		}
		if (GetTickCount64() - tick1 > 20)
		{

			std::string str;
			lstString.PopFront(str);
			
			tick1 = GetTickCount64();
		}
		
	}
	printf("exit done!size %d\r\n", lstString.Size());
	lstString.Clear();
	printf("exit done!size %d\r\n", lstString.Size());
}

void initsock()
{
	WSAData data;
	WSAStartup(MAKEWORD(2, 2), &data);
}

void clearsock()
{
	WSACleanup();
}

//int RecvFromCB(void* arg, const EBuffer& buffer, ESockaddrIn& addr)
//{
//	ECServer* server = (ECServer*)arg;
//	return server->Sendto(addr,buffer);
//}
//
//int SendToCB(void*arg)
//{
//	ECServer* server = (ECServer*)arg;
//
//	return 0;
//}

//#include "ENetwork.h"
//void udp_server()
//{ 
//	EServerParameter param(
//		"127.0.0.1", 20000,ETYPE::ETypeUDP,NULL,NULL,NULL,RecvFromCB,SendToCB
//		);
//	ECServer server(param);
//	server.Invoke(NULL);
//	printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
//	getchar();
//	return;
//	ESOCKET sock(new ESocket(ETYPE::ETypeUDP));
//	if (*sock == INVALID_SOCKET)
//	{
//		return;
//	}
//	std::list<ESockaddrIn>lstclients;
//	ESockaddrIn client;
//	if (-1 == sock->bind("127.0.0.1", 20000))
//	{
//		closesocket(*sock);
//		return;
//	}
//	EBuffer buf(1024*256);
//	int len = sizeof(client);
//	int ret = 0;
//	while (!_kbhit())
//	{
//		ret = sock->recvfrom(buf, client);
//		if(ret>0)
//		{
//			client.update();																		
//			if (lstclients.size() <= 0)
//			lstclients.push_back(client);
//			ret = sock->sendto(buf,client);
//		}
//		else
//		{
//			buf.Update((void*)&lstclients.front(), lstclients.front().size());
//			ret = sock->sendto(buf, client);
//		}
//		Sleep(1);
//	}
//	getchar();
//
//}
//
//void udp_client(bool ishost=true)
//{
//	
//	Sleep(2000);
//	sockaddr_in server, client;
//	int len = sizeof(client);
//	server.sin_family = AF_INET;
//	server.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
//	server.sin_port = htons(20000);
//	SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
//	if (sock == INVALID_SOCKET)
//	{
//		
//		return;
//	}
//	if (ishost)//主客户端
//	{
//		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
//		EBuffer msg;
//		int ret=sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
//		if (ret > 0)
//		{
//			msg.resize(1024);
//			memset((char*)msg.c_str(), 0, msg.size());
//			ret=recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client,&len);
//			printf("%s\r\n", msg.c_str());
//			ret = recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);
//		}
//	}
//	else//从客户端
//	{
//		printf("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
//		std::string msg = "hello world!\n";
//		int ret = sendto(sock, msg.c_str(), msg.size(), 0, (sockaddr*)&server, sizeof(server));
//		if (ret > 0)
//		{
//			msg.resize(1024);
//			memset((char*)msg.c_str(), 0, msg.size());
//			ret=recvfrom(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&client, &len);	
//			if (ret > 0)
//			{
//				sockaddr_in addr;
//				memcpy(&addr, msg.c_str(), sizeof(addr));		
//				sockaddr_in* paddr = (sockaddr_in*)&addr;
//				msg = "hello i am client\r\n";
//				ret= sendto(sock, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)paddr, sizeof(sockaddr_in));
//			}
//		}
//	}
//	closesocket;
//}


int main(int argc,char* argv[])
{
	/*initsock();*/
	if (!Init())return 1;
	/*if (argc == 1)
	{
		char wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH, wstrDir);
		STARTUPINFOA si;
		PROCESS_INFORMATION pi;
		memset(&si, 0, sizeof(si));
		memset(&pi, 0, sizeof(pi));
		string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
		if (bRet)
		{
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("进程ID：%d\r\n", pi.dwThreadId);
			TRACE("线程ID：%d\r\n", pi.dwProcessId);
			strCmd += " 2"; 
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet)
			{
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("进程ID：%d\r\n", pi.dwThreadId);
				TRACE("线程ID：%d\r\n", pi.dwProcessId);
				udp_server();
			}
		}
		
	}
	else if (argc == 2)
	{
		udp_client();
	}
	else
	{
		udp_client(false);
	}
	clearsock();*/
	//printf("press any key to exit ...\r\n");
	//HANDLE hIOCP = INVALID_HANDLE_VALUE;//io completion port
	//hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
	//if (hIOCP == NULL)
	//{
	//	printf("create iocp failed!\r\n", GetLastError());
	//}
	//HANDLE hThread = (HANDLE)_beginthread(threadQueryEntry, 0, hIOCP);
	//ULONGLONG tick = GetTickCount64();
	//while (_kbhit()==0)//检查键盘是否有按键输入
	//{
	//	if (GetTickCount64() - tick > 1300)
	//	{
	//		PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPop, "hello world",func), NULL);
	//		tick = GetTickCount64();
	//	}
	//	if(GetTickCount64()-tick>2000)
	//	{
	//		PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_PARAM), (ULONG_PTR)new IOCP_PARAM(IocpListPush, "hello world"), NULL);
	//		tick = GetTickCount64();
	//	}
	//	Sleep(1);
	//}
	//if (hIOCP != NULL)
	//{
	//	PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);//发送一个空消息代表结束
	//	WaitForSingleObject(hThread, INFINITE);
	//}
	//CloseHandle(hIOCP);
	//printf("exit done!\r\n");
	/*EServer server;
	server.StartService();
	getchar();*/


	iocpServer server;
	server.RunServer();
	/*CCommand cmd;
	int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
	switch (ret)
	{
	case -1:
		MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态！"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
		exit(0);
		break;
	case -2:
		MessageBox(NULL, _T("多次无法正常接入用户，结束程序！"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
		exit(0);
		break;
	}*/

	return 0;
}



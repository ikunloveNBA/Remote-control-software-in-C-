#pragma once
#include<map>
#include"ServerSocket.h"
#include<atlimage.h>
#include<direct.h>
#include"CTool.h"
#include"LockInfoDialog.h"
#include"Resource.h"
#include"Packet.h"
#include<io.h>
#include<list>

class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int	ExcuteCommand(int nCmd, std::list<CPacket>&,CPacket&);
	static void RunCommand(void* arg, int status,std::list<CPacket>&lstPacket, CPacket&intPacket)
	{
		CCommand* thiz = (CCommand*)arg;
		if (status > 0)
		{
			int ret=thiz->ExcuteCommand(status, lstPacket,intPacket);
			if ( ret!= 0)
			{
				TRACE("����ִ��ʧ��:%d ret=%d\r\n", status, ret);

			}

		}
		else
		{
			MessageBox(NULL, _T("�޷����������û����Զ����ԣ�"), _T("�����û�ʧ�ܣ�"), MB_OK | MB_ICONERROR);
			exit(0);
		}
	}

protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&,CPacket&inPacket);//��Ա����ָ��
	std::map<int, CMDFUNC>m_mapFunction;//������ŵ����ܵ�ӳ��
	CLockInfoDialog dlg;
	unsigned threadid;
protected:
	//�����̺߳���
	static unsigned _stdcall threadLockDlg(void* arg)
	{
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	void threadLockDlgMain()
	{
		dlg.Create(IDD_DIALOG_INFO, NULL);
		dlg.ShowWindow(SW_SHOW);
		//�ڱκ�̨����
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = GetSystemMetrics(SM_CXFULLSCREEN);
		//rect.bottom = LONG(rect.bottom * 1.10);
		dlg.MoveWindow(rect);
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);//�õ��������ڵ�����
		if (pText)
		{
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width();
			int x = (rect.right - nWidth) / 2;
			int nHeight = rtText.Height();
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}
		//�����ö�
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		//������깦��
		ShowCursor(false);
		//����������
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);//
		rect.left = 0;
		rect.top = 0;
		rect.bottom = 1;
		rect.right = 1;
		ClipCursor(rect);//���������Χ
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN)
			{
				if (msg.wParam == 0x41)//��a���˳�
					break;
			}
		}

		ClipCursor(NULL);//�ָ������Χ
		ShowCursor(true);//�ָ����
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);//�ָ�������
		dlg.DestroyWindow();
	}
	//����������Ϣ
	int MakeDriverInfo(std::list<CPacket>&lstPacket, CPacket& inPacket)
	{
		std::string result;
		for (int i = 1; i <= 26; i++)
		{
			if (_chdrive(i) == 0)
			{
				if (result.size() > 0)
				{
					result += ',';
				}
				result += 'A' + i - 1;
			}
		}
		//result+=",";
		lstPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
		return 0;
	}




	//��ȡָ���ļ����µ���Ϣ
	int MakeDirectoryInfo(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;

		TRACE("%s\n", strPath.c_str());
		if (_chdir(strPath.c_str()) != 0)
		{
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			OutputDebugString(_T("û�з���Ȩ�ޣ�����Ŀ¼��"));
			return -2;
		}
		_finddata_t fdata;
		intptr_t hfind = 0;
		hfind = _findfirst("*", &fdata);
		if (hfind == -1)
		{
			OutputDebugString(_T("û���ҵ��κ��ļ���"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			return -3;
		}
		do
		{
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;//�жϸ��ļ��ǲ���Ŀ¼
			memcpy(finfo.szFileName, fdata.name, strlen(fdata.name));
			TRACE("%s\n", fdata.name);
			lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			//Sleep(1);//��ʱ�����ͻ��˵���ؼ���Ҫʱ��
		} while (_findnext(hfind, &fdata) == 0);

		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));

		return 0;
	}

	int RunFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
		lstPacket.push_back(CPacket(3, NULL, 0));
		return 0;
	}

	int DownloadFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		long long data = 0;
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if (err != 0)
		{
			//�ļ���ʧ��
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			return -1;
		}
		if (pFile != NULL)
		{
			//�ȷ����ļ����ݵ��ܳ���
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			lstPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do
			{
				//ѭ�������ļ�����
				rlen = fread(buffer, 1, 1024, pFile);
				lstPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
				//TRACE("%s\r\n", buffer);
			} while (rlen >= 1024);

			fclose(pFile);
		}
		//������ϵ��ź�
		lstPacket.push_back(CPacket(4, NULL, 0));
		return 0;
	}

	int MouseEvent1(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));

		
			DWORD nFlags = 0;
			switch (mouse.nButton)
			{
			case 0://���
				nFlags = 1;
				break;
			case 1://�Ҽ�
				nFlags = 2;
				break;
			case 2://�м�
				nFlags = 4;
				break;
			case 8://û�а���
				nFlags = 8;
				break;
			}
			if (nFlags != 8)SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
			switch (mouse.nAction)
			{
			case 0://����
				nFlags |= 0x10;
				break;
			case 1://˫��
				nFlags |= 0x20;
				break;
			case 2://����
				nFlags |= 0x40;
				break;
			case 3://�ſ�
				nFlags |= 0x80;
				break;
			}

			switch (nFlags)
			{
			case 0x21://���˫��
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x11://�������
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x41://�������
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x81://����ſ�
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x22://�Ҽ�˫��
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x12://�Ҽ�����
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x42://�Ҽ�����
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x82://�Ҽ��ſ�
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x24://�м�˫��
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x14://�м�����
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x44://�м�����
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x84://�м��ſ�
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x08://����ƶ�
				mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
				break;
			}
			lstPacket.push_back(CPacket(5, NULL, 0));
		
		return 0;
	}

	int SendScreen(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		CImage screen;//GDI
		HDC hScreen = GetDC(NULL);//��ȡ��Ļ�豸��������
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);//��ȡ��Ļ��ÿ����λ����λ��ȣ�
		//BITSPIXEL			ÿ������������ɫ��λ��
		int nWidth = GetDeviceCaps(hScreen, HORZRES);
		int nHeight = GetDeviceCaps(hScreen, VERTRES);
		screen.Create(nWidth, nHeight, nBitPerPixel);
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);
		ReleaseDC(NULL, hScreen);
		DWORD64 tick = GetTickCount64();//��ȡ���������ڵ�ʱ��
		//screen.Save(_T("test2020.png"), Gdiplus::ImageFormatPNG);//png���С����jpg�ٶȿ죬Ȩ��֮��ѡ��png
		/*TRACE("png %d\r\n", GetTickCount64() - tick);
		tick = GetTickCount64();
		screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
		TRACE("jpg %d\r\n", GetTickCount64() - tick);*/

		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, NULL);
		if (hMem == NULL)return -1;
		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMem, TRUE, &pStream);
		if (ret == S_OK)
		{
			screen.Save(pStream, Gdiplus::ImageFormatPNG);
			LARGE_INTEGER bg = { 0 };
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);
			PBYTE pData = (PBYTE)GlobalLock(hMem);
			SIZE_T nSize = GlobalSize(hMem);
			lstPacket.push_back(CPacket(6, pData, nSize));
			GlobalUnlock(hMem);
		}
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}



	int LockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE))
		{
			_beginthreadex(NULL, 0,&CCommand::threadLockDlg, this, 0, &threadid);
		}
		lstPacket.push_back(CPacket(7, NULL, 0));
		return 0;
	}

	int UnlockMachine(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		//::SendMessage(dlg.m_hWnd,WM_KEYDOWN, 0X41, 0X01E0001);
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
		lstPacket.push_back(CPacket(8, NULL, 0));
		return 0;
	}

	int TestConnect(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		lstPacket.push_back(CPacket(1981, NULL, 0));
		return 0;
	}

	int DeleteLocalFile(std::list<CPacket>& lstPacket, CPacket& inPacket)
	{
		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		//ת�ɿ��ֽ�
		//mbstowcs(sPath, strPath.c_str(), strPath.size());
		MultiByteToWideChar(
			CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
			sizeof(sPath) / sizeof(TCHAR)
		);
		DeleteFileA(strPath.c_str());
		lstPacket.push_back(CPacket(9, NULL, 0));

		return 0;
	}
};


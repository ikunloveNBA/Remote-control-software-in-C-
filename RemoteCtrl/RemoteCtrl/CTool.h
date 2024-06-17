#pragma once
class CCTool
{
public:
	static void Dump(BYTE* pData, size_t nSize)
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
	//��ӡ������Ϣ
	static void ShowError()
	{
		LPWSTR lpMessageBuf = NULL;
		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
			NULL, GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPWSTR)&lpMessageBuf, 0, NULL);
		OutputDebugString(lpMessageBuf);
		LocalFree(lpMessageBuf);
		::exit(0);
	}

	//�ж��߳�Ȩ�ޣ��Ƿ��ǹ���Ա��
	static bool IsAdmin()
	{
		HANDLE hToken = NULL;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))//��ȡ���̵߳ķ�������
		{
			ShowError();
			return false;
		}
		TOKEN_ELEVATION eve;
		DWORD len = 0;
		if (GetTokenInformation(hToken, TokenElevation, &eve, sizeof(eve), &len) == FALSE)//��ȡ��ǰ�߳��Ƿ�������Ȩ�޵���Ϣ
		{
			//����ȡ����Ϣ����eve��
			ShowError();
			return false;
		}
		CloseHandle(hToken);
		if (len == sizeof(eve))//�жϻ�ȡ��eve�ĳ��Ⱥ�len�Ƿ����
		{
			return eve.TokenIsElevated;
		}
		printf("length of tokeninformation is %d\r\n", len);
		return false;
	}

	static bool RunAsAdmin()
	{
		//���ز����飬��������Ա�˻� ��ֹ������ֻ�ܵ�½���ؿ���̨

		STARTUPINFO si = { 0 };
		PROCESS_INFORMATION pi = { 0 };
		TCHAR sPath[MAX_PATH] = _T("");
		GetModuleFileName(NULL, sPath, MAX_PATH);
		//�����½��̣��Թ���Աģʽ��
		BOOL ret = CreateProcessWithLogonW(_T("Administrator"), NULL, NULL, LOGON_WITH_PROFILE, NULL, (LPWSTR)(LPCWSTR)sPath, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &si, &pi);
		if (!ret)
		{
			ShowError();
			MessageBox(NULL, _T("��������ʧ��"), _T("�������"), 0);
			return false; 
		}
		WaitForSingleObject(pi.hProcess, INFINITE);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}

};


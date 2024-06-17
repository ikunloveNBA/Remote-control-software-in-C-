#pragma once
#include<atomic>
#include<vector>
#include<mutex>
#include"pch.h"

class ThreadFuncBase//�̹߳��������Ļ���
{
	//ThreadWorker�๦��ʵ��ʵ�������ĺ�����
};

typedef int (ThreadFuncBase::* FUNCTYPE)();
class ThreadWorker//���̳߳�Ͷ�ݵ�worker��
{
public:
	ThreadWorker() :thiz(NULL), func(NULL) {}
	ThreadWorker(void* obj, FUNCTYPE f) :thiz((ThreadFuncBase*)obj), func(f) {}
	ThreadWorker(const ThreadWorker& worker)
	{
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker)
	{
		if(this!=&worker)
		thiz = worker.thiz;
		func = worker.func;
		return *this;
	}
	int operator()()
	{
		if (IsValid())
		{
			return (thiz->*func)();
		}
		return -1;
	}
	bool IsValid()const
	{
		return ((thiz != NULL) && (func != NULL));
	}
private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};

class CCThread
{
public:
	CCThread()
	{
		m_hThread = NULL;
		m_Status = false;
	}
	~CCThread() 
	{
		Stop();
	}

	bool Start()//�����߳�
	{
		//true��ʾ�ɹ���false��ʾʧ��
		m_Status = true;
		m_hThread = (HANDLE)_beginthread(&CCThread::ThreadEntry, 0, this);
		if (!IsValid())
		{
			m_Status = false;
		}
		return m_Status;
	}
	bool IsValid()//true �߳�����Ч��
	{
		if (m_hThread == NULL || m_hThread == INVALID_HANDLE_VALUE)return false;
		return WaitForSingleObject(m_hThread,0) == WAIT_TIMEOUT;
	}

	bool Stop()//ֹͣ�߳�
	{
		if (m_Status == false)return true;
		m_Status = false;
		DWORD ret = WaitForSingleObject(m_hThread, 1000);
		if (ret == WAIT_TIMEOUT)
		{
			TerminateThread(m_hThread, -1);
		}

		UpdateWorker();
		return ret == WAIT_OBJECT_0; 
	}

	void UpdateWorker(const ::ThreadWorker& worker=::ThreadWorker())
	{
		if (m_worker.load() != NULL&&(m_worker.load()!=&worker))
		{
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker)return;
		if (!worker.IsValid())
		{
			m_worker.store(NULL);
			return;
		}
		::ThreadWorker* pWorker = new ::ThreadWorker(worker);
		m_worker.store(pWorker);
	}

	//true��ʾ���� false�Ѿ������˹���
	bool IsIdle()//�Ƿ����
	{
		if (m_worker.load() == NULL)return true;
		return !m_worker.load()->IsValid();
	}
	
private: 
	virtual void ThreadWorker()
	{
		while (m_Status)
		{
			if (m_worker.load() == NULL)
			{
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (m_worker.load()->IsValid())
			{
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT)
				{

				}
				int ret = worker();
				if (ret != 0)
				{
					CString str;
					str.Format(_T("thread found error code��%d\r\n", ret));
					OutputDebugString(str);
				}
				if (ret < 0)
				{
					::ThreadWorker* pWorker = m_worker.load();
					m_worker.store(new ::ThreadWorker());
					delete pWorker;
				}
				
			}
			else
			{
				Sleep(1);
			}
		}
	}
	static void ThreadEntry(void* arg)
	{
		CCThread*thiz = (CCThread*)arg;
		thiz->ThreadWorker();
	}
private:
	HANDLE m_hThread;
	bool m_Status;//false ��ʾ�߳̽�Ҫ���� true ��ʾ�߳���������
	std::atomic<::ThreadWorker*>m_worker;
};

class CCThreadPool
{
public:
	CCThreadPool(size_t size)
	{
		m_threads.resize(size);
		for (size_t i = 0; i < size; i++)
		{
			m_threads[i] = new CCThread();
		}
	}
	CCThreadPool() {}
	~CCThreadPool() 
	{
		Stop();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			delete m_threads[i];
			m_threads[i] = NULL;
		}
		m_threads.clear();
	}
	bool Invoke()
	{
		bool ret = true;
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads[i]->Start()==false)
			{
				ret = false;//ֻҪ��һ���߳�����ʧ�ܣ���ô�Ҿ�Ҫ�ر������̣߳����������̶߳������ɹ�����ɹ�
			}
		}
		if (ret == false)
		{
			for (size_t i = 0; i < m_threads.size(); i++)
			{
				m_threads[i]->Stop();
			}
		}
		return ret;
	}

	void Stop()
	{
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			m_threads[i]->Stop();
		}
	}

	int DispatchWorker(const ThreadWorker& worker)
	{
		//����-1���е��̶߳���æ������ֵ>0�Ƿ�����̵߳��±�
		int index = -1;
		m_lock.lock();
		for (size_t i = 0; i < m_threads.size(); i++)
		{
			if (m_threads[i]->IsIdle())
			{
				m_threads[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		return index;
	}

	bool CheckThreadValid(size_t index)
	{
		if (index < m_threads.size())
		{
			return m_threads[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<CCThread*>m_threads;
};
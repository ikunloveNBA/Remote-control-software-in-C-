#pragma once
#include"pch.h"
#include<atomic>
#include<list>
#include"CCThread.h"
template<class T>
class CQueue//�̰߳�ȫ���У�����iocp��
{
public:
	enum CQMode
	{
		EQNone,
		EQPush,
		EQPop,
		EQSize,
		EQClear
	};
	typedef struct IocpParam
	{
		size_t nOperator;//����
		T Data;//����
		HANDLE hEvent;//pop������Ҫ��
		IocpParam(int op, const T& data,HANDLE hEve=NULL)
		{
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		~IocpParam()
		{
			nOperator = EQNone;
		}
	}PPARAM;//Post Parameter ����Ͷ����Ϣ�Ľṹ��
	
public:
	CQueue()
	{
		m_lock = false;
		m_hCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompletionPort != NULL)
		{
			m_hThread = (HANDLE)_beginthread(&CQueue<T>::threadEntry, 0, this);
		}
	}
	virtual ~CQueue()
	{
		if (m_lock)return;
		m_lock=true;
		PostQueuedCompletionStatus(m_hCompletionPort, 0, NULL, NULL);//���Ϳ���Ϣ������ͣ
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompletionPort != NULL)
		{
			HANDLE hTemp = m_hCompletionPort;
			m_hCompletionPort = NULL;
			CloseHandle(hTemp);
		}
		
	}
	bool PushBack(const T& data)
	{
		//����������ʱpush������Ҫ��ԭ�Ӳ���
		IocpParam* pParam = new IocpParam(EQPush,data);
		if (m_lock)
		{
			delete pParam;
			return false;
		}
		bool ret=PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)
		{
			delete pParam;
		}
		return ret;
	}
	virtual bool PopFront(T& data)
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam pParam(EQPop, data,hEvent);
		if (m_lock)
		{
			if (hEvent)CloseHandle(hEvent);
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
		if (ret == false)
		{
			CloseHandle(hEvent);
			return false;
		}
		ret=WaitForSingleObject(hEvent,INFINITE)==WAIT_OBJECT_0;//����ȵ������ֵ��������ʾ���ź�״̬
		if (ret)
		{
			data = pParam.Data;
		}
		return ret;
	}
	size_t Size()
	{
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam pParam(EQSize, T(), hEvent);
		if (m_lock)
		{
			if (hEvent)CloseHandle(hEvent); 
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)&pParam, NULL);
		if (ret == false)
		{
			CloseHandle(hEvent);
			return -1;
		}
		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;//����ȵ������ֵ��������ʾ���ź�״̬
		if (ret)
		{
			return pParam.nOperator;//����������Ϊ���д�С����
		}
		return -1;
	}
	void Clear()
	{
		if (m_lock)
		{
			return;
		}
		IocpParam* pParam = new IocpParam(EQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)
		{
			delete pParam;
			return;
		}
		return ;
	}
protected:
	static void threadEntry(void* arg)
	{
		CQueue<T>* thiz = (CQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}
	virtual void DealParam(PPARAM* pParam)
	{
		switch (pParam->nOperator)
		{
		case EQPush:
		{
			m_listData.push_back(pParam->Data);
			delete pParam;
			break;
		}
		case EQPop:
		{
			if (m_listData.size() > 0)
			{
				pParam->Data = m_listData.front();
				m_listData.pop_front();
			}
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent);
			}
			break;
		}
		case EQSize:
		{
			pParam->nOperator = m_listData.size();
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent);
			}
			break;
		}
		case EQClear:
		{
			m_listData.clear();
			delete pParam;
			break;
		}
		default:
			break;
		}
	}
	virtual void threadMain()
	{
		DWORD dwTransferred = 0;
		PPARAM* pParam = NULL;
		ULONG_PTR CompetionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(
			m_hCompletionPort,
			&dwTransferred,
			&CompetionKey,
			&pOverlapped,
			INFINITE
		))
		{
			if (dwTransferred == 0 || CompetionKey == NULL)
			{
				printf("thread is prepare to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompetionKey;
			DealParam(pParam);
		}
		//ΪʲôҪ�ٴ���һ�Σ���Ϊ�������У�������postNULL��ʱ��ͬʱ�������߳�Ҳpost���ݣ���ʱ�ں˶��л��в�������δ����
		while (GetQueuedCompletionStatus(
			m_hCompletionPort,
			&dwTransferred,
			&CompetionKey,
			&pOverlapped,
			0
		))
		{
			if (dwTransferred == 0 || CompetionKey == NULL)
			{
				printf("thread is prepare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompetionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompletionPort;
		m_hCompletionPort = NULL;
		CloseHandle(hTemp);
	}
protected:
	std::list<T>m_listData;
	HANDLE m_hCompletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;//������������

};


template<class T>
class ESendQueue :public CQueue<T>,public ThreadFuncBase
{
public:
	typedef int (ThreadFuncBase::* ECALLBACK)(T&data);

	ESendQueue(ThreadFuncBase* obj, ECALLBACK callback) :CQueue<T>(),
		m_base(obj), m_callback(callback) 
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this,(FUNCTYPE) & ESendQueue<T>::threadTick));
	}
	virtual ~ESendQueue() 
	{
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}
	virtual bool PopFront(T& data)
	{
		return false;
	}
	bool PopFront()
	{
		typename CQueue<T>::IocpParam* pParam=new typename CQueue<T>::IocpParam(CQueue<T>::EQPop, T());
		if (CQueue<T>::m_lock)
		{
			delete pParam;
			return -1;
		}
		bool ret = PostQueuedCompletionStatus(CQueue<T>::m_hCompletionPort, sizeof(*pParam), (ULONG_PTR)&pParam, NULL);
		if (ret == false)
		{
			delete pParam;
			return false;
		}
		return ret;
	}

protected:
	int threadTick() 
	{
		if (WaitForSingleObject(CQueue<T>::m_hThread, 0) != WAIT_TIMEOUT)return 0;
		if (CQueue<T>::m_listData.size() > 0)
		{
			PopFront();
		}
		
		return 0;
	}
	virtual void DealParam(typename CQueue<T>::PPARAM* pParam)
	{
		switch (pParam->nOperator)
		{
		case CQueue<T>::EQPush:
		{
			CQueue<T>::m_listData.push_back(pParam->Data);
			delete pParam;
			break;
		}
		case CQueue<T>::EQPop:
		{
			if (CQueue<T>::m_listData.size() > 0)
			{
				pParam->Data = CQueue<T>::m_listData.front();
				if((m_base->*m_callback)(pParam->Data)==0)
					CQueue<T>::m_listData.pop_front();
			}
			delete pParam;
			break;
		}
		case CQueue<T>::EQSize:
		{
			pParam->nOperator = CQueue<T>::m_listData.size();
			if (pParam->hEvent != NULL)
			{
				SetEvent(pParam->hEvent);
			}
			break;
		}
		case CQueue<T>::EQClear:
		{
			CQueue<T>::m_listData.clear();
			delete pParam;
			break;
		}
		default:
			break;
		}
	}
private:
	ThreadFuncBase* m_base;
	ECALLBACK m_callback;
	CCThread m_thread;
};
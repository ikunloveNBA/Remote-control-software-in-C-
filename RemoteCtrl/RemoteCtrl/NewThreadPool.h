#pragma once
#include<thread>
#include<mutex>
#include<condition_variable>
#include<memory>
#include<atomic>
#include<future>
#include<queue>
#include<vector>

class NewThreadPool
{
public:
	typedef std::packaged_task<void()> Task;
	NewThreadPool(const NewThreadPool&) = delete;
	NewThreadPool& operator=(const NewThreadPool&) = delete;
	static NewThreadPool& instance()
	{
		static NewThreadPool ins;
		return ins;
	}
	~NewThreadPool() 
	{
		Stop();
	}

	template<typename F,typename... Args>
	auto Commit(F&&f,Args&&...args)->std::future<decltype(f(args...))>
	{
		using RetType = decltype(f(args...));
		if (m_stop.load())
		{
			return std::future<RetType>{};
		}
		auto task = std::make_shared<std::packaged_task<RetType()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
		std::future < RetType >ret = task->get_future();
		{
			std::lock_guard<std::mutex>lock(m_mt);
			m_tasks.emplace([task]() {(*task)(); });
		}
		m_lock.notify_one();
		return ret;
	}

	int idleThreadCount()
	{
		return m_thread_nums;
	}
private:
	NewThreadPool(unsigned int nums=5) :m_stop(false)
	{
		if (nums < 1)
		{
			m_thread_nums = 1;
		}
		else
		{
			m_thread_nums = nums;
		}
		Start();
	}
	void Start()
	{
		for (int i = 0; i < m_thread_nums; ++i)
		{
			m_threads.emplace_back([this]()
				{
					while (!this->m_stop.load())
					{
						Task task;
						{
							//没有任务就将线程挂起，避免浪费资源
							std::unique_lock<std::mutex>lock(m_mt);
							this->m_lock.wait(lock, [this]()
								{return this->m_stop.load() || !this->m_tasks.empty(); });
							if (m_tasks.empty())
							{
								return;
							}
							task = std::move(this->m_tasks.front());
							this->m_tasks.pop();
						}
						this->m_thread_nums--;
						task();
						this->m_thread_nums++;
					}
				});
		}
	}

	void Stop()
	{
		m_stop.store(true);
		m_lock.notify_all();
		for (auto& t : m_threads)
		{
			if(t.joinable())
			t.join();
		}
	}
private:
	std::vector<std::thread>		m_threads;//线程数
	std::queue<Task>				m_tasks;//任务队列
	std::atomic_bool				m_stop;//线程池停止消息
	std::mutex						m_mt;//锁
	std::condition_variable			m_lock;//状态变量
	std::atomic_uint				m_thread_nums;//空闲线程的数量
};


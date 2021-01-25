#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <queue>
#include <iostream>
#include "MyUNP.h"
#include "synchronize.h"

template<typename Task>
class threadpool {
public:
	threadpool(int nthread = 4, int max_ntask = 1024) :
		m_nthread(nthread), m_max_ntask(max_ntask), m_stop(false),
		m_threads(nullptr) {
		if (nthread <= 0 || max_ntask <= 0) throw std::exception();
		m_threads = new pthread_t[m_nthread];

		for (int i = 0; i < m_nthread; ++i) {
			std::cout << "thread " << i << " create" << std::endl;
			if (pthread_create_detached(&m_threads[i], worker_thread, this) != 0) {
				delete[] m_threads;
				throw std::exception();
			}
		}
	}
	threadpool(const threadpool&) = delete;
	~threadpool() { delete[] m_threads; m_stop = true; }

	/* 供主线程使用，当有新的事件到达时可以将其封装成任务对象
		后加入到工作请求队列让工作线程对其进行处理 */
	bool append(Task* task) {
		m_queuelocker.lock();
		if (m_workqueue.size() > m_max_ntask) {
			m_queuelocker.unlock();
			return false;
		}
		m_workqueue.push(task);
		m_queuelocker.unlock();
		m_queuestat.post();
		return true;
	}

private:
	static void* worker_thread(void* args);
	void run();

private:
	std::queue<Task*> m_workqueue;
	pthread_t* m_threads;
	Locker m_queuelocker;
	Sem m_queuestat;

	int m_nthread;
	int m_max_ntask;
	int m_stop;
};


template<typename Task>
void* threadpool<Task>::worker_thread(void* args) {
	threadpool<Task>* pool = static_cast<threadpool<Task>*>(args);
	pool->run();
	return pool;
}

template<typename Task>
void threadpool<Task>::run() {
	while (!m_stop) {
		m_queuestat.wait();
		m_queuelocker.lock();
		if (m_workqueue.empty()) {
			m_queuelocker.unlock();
			continue;
		}
		Task* task = m_workqueue.front();
		m_workqueue.pop();
		m_queuelocker.unlock();
		if (!task) continue;
		task->process();
	}
}



#endif // !THREADPOOL_H_

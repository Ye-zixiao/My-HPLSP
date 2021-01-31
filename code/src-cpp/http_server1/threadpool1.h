#ifndef THREADPOOL1_H_
#define THREADPOOL1_H_

#include <list>
#include <iostream>
#include <exception>
#include <pthread.h>

/* 工作队列 */
template<typename Task>
class workqueue {
public:
	workqueue() {
		if (pthread_mutex_init(&m_lock, nullptr) != 0)
			throw std::exception();
		if (pthread_cond_init(&m_cond, nullptr) != 0) {
			pthread_mutex_destroy(&m_lock);
			throw std::exception();
		}
	}
	~workqueue() {
		pthread_mutex_destroy(&m_lock);
		pthread_cond_destroy(&m_cond);
	}

	size_t size() const { return m_tasklist.size(); }
	bool empty() const { return m_tasklist.empty(); }
	bool lock() { return pthread_mutex_lock(&m_lock) ? false : true; }
	bool unlock() { return pthread_mutex_unlock(&m_lock) ? false : true; }
	bool condwait() { return pthread_cond_wait(&m_cond, &m_lock) ? false : true; }
	bool condsignal() { return pthread_cond_signal(&m_cond) ? false : true; }
	bool condbroadcast() { return pthread_cond_broadcast(&m_cond) ? false : true; }
	Task* front_n_pop() {
		if (empty()) return nullptr;
		Task* ret = m_tasklist.front();
		m_tasklist.pop_front();
		return ret;

	}
	void append(Task* task) {
		m_tasklist.push_back(task);
	}

private:
	std::list<Task*> m_tasklist;
	pthread_mutex_t m_lock;
	pthread_cond_t m_cond;
};

/* 线程池 */
template<typename Task>
class threadpool {
public:
	threadpool(size_t nthreads = 4, size_t max_ntask = 1024);
	~threadpool() {
		m_stop = true;
		m_workqueue.condbroadcast();
		delete[] m_threads;
	}

	bool append(Task* task);

private:
	static void* worker_thread(void* args);
	void run();

private:
	workqueue<Task> m_workqueue;
	size_t m_max_ntask;
	pthread_t* m_threads;
	size_t m_nthread;
	bool m_stop;
};


template<typename Task>
inline threadpool<Task>::threadpool(size_t nthreads, size_t max_ntask) :
	m_max_ntask(max_ntask), m_nthread(nthreads), m_stop(false) {
	if (nthreads <= 0 || max_ntask <= 0) throw std::exception();
	m_threads = new pthread_t[nthreads];

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	for (decltype(nthreads) i = 0; i < nthreads; ++i) {
		std::cout << "create thread " << i << std::endl;
		if (pthread_create(&m_threads[i], &attr, worker_thread, this) != 0) {
			pthread_attr_destroy(&attr);
			delete[] m_threads;
			throw std::exception();
		}
	}
	pthread_attr_destroy(&attr);
}

template<typename Task>
inline bool threadpool<Task>::append(Task* task) {
	m_workqueue.lock();
	if (m_workqueue.size() >= m_max_ntask) {
		m_workqueue.unlock();
		return false;
	}
	m_workqueue.append(task);
	m_workqueue.unlock();
	m_workqueue.condsignal();
	return true;
}

template<typename Task>
inline void threadpool<Task>::run() {
	while (!m_stop) {
		m_workqueue.unlock();
		if (m_workqueue.empty())
			m_workqueue.condwait();
		Task* task = m_workqueue.front_n_pop();
		m_workqueue.unlock();
		if (task) task->process();
	}
}

template<typename Task>
void* threadpool<Task>::worker_thread(void* args) {
	threadpool<Task>* pool = (threadpool<Task>*)args;
	pool->run();
	return pool;
}


#endif // !THREADPOOL1_H_

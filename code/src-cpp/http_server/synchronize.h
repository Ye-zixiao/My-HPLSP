#ifndef SYNCHRONIZE_H_
#define SYNCHRONIZE_H_

#include <pthread.h>
#include <semaphore.h>
#include <exception>

/**
 * 简单的封装pthread同步对象
 */

//信号量的封装类
class Sem {
public:
	Sem() {
		if (sem_init(&m_sem, PTHREAD_PROCESS_PRIVATE, 0) != 0)
			throw std::exception();
	}
	~Sem() { sem_destroy(&m_sem); }

	bool wait() { return sem_wait(&m_sem) == 0; }
	bool post() { return sem_post(&m_sem) == 0; }
	
private:
	sem_t m_sem;
};


//互斥量的封装类
class Locker {
public:
	Locker() {
		if (pthread_mutex_init(&m_mutex, NULL) != 0)
			throw std::exception();
	}
	~Locker() { pthread_mutex_destroy(&m_mutex); }

	bool lock() { return pthread_mutex_lock(&m_mutex) == 0; }
	bool unlock() { return pthread_mutex_unlock(&m_mutex) == 0; }

private:
	pthread_mutex_t m_mutex;
};


//条件变量的封装类
class Cond {
public:
	Cond() {
		if (pthread_mutex_init(&m_cond_mutex, NULL) != 0)
			throw std::exception();
		if (pthread_cond_init(&m_cond, NULL) != 0) {
			pthread_mutex_destroy(&m_cond_mutex);
			throw std::exception();
		}
	}
	~Cond() {
		pthread_cond_destroy(&m_cond);
		pthread_mutex_destroy(&m_cond_mutex);
	}

	bool wait() {
		int ret;
		pthread_mutex_lock(&m_cond_mutex);
		ret = pthread_cond_wait(&m_cond, &m_cond_mutex);
		//另一种策略是在cond_signal()之后解锁
		pthread_mutex_unlock(&m_cond_mutex);
		return ret == 0;
	}
	bool signal() { return pthread_cond_signal(&m_cond); }

private:
	pthread_cond_t m_cond;
	//该互斥量只负责条件变量的保护
	pthread_mutex_t m_cond_mutex;
};



#endif // !SYNCHRONIZE_H_
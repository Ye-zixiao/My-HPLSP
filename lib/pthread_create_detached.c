#include "MyUNP.h"


/**
 * 创建一个可分离状态的线程
 */
int pthread_create_detached(pthread_t* thread,
		void* (routine)(void*), void* arg) {
	pthread_attr_t attr;
	int err;

	if ((err = pthread_attr_init(&attr)) != 0)
		return err;
	if ((err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) != 0)
		return err;
	if ((err = pthread_create(thread, &attr, routine, arg)) != 0)
		return err;
	if ((err = pthread_attr_destroy(&attr)) != 0)
		return err;
	return 0;
}
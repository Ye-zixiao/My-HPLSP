#include "MyUNP.h"


static void sig_alrm(int signo) {
	return;
}


/** 
 * 执行connect函数，但必须要求其在指定时间内完成连接的建立，否则由SIGALRM
 * 信号中断并设置ETIMEDOUT，而不是使用按照connect自己的规定等待超时
 */
int connect_timeo(int sockfd, const struct sockaddr* svaddr, socklen_t len, time_t nsec) {
	__sighandler_t old_sigfunc;
	int err;

	if ((old_sigfunc = mysignal(SIGALRM, sig_alrm)) == SIG_ERR)
		return -1;
	if (alarm(nsec) != 0)
		err_msg("connect_timeo: alarm was already set");
	if ((err = connect(sockfd, svaddr, len)) == -1) {
		if (close(sockfd) == -1)
			err_sys("close error");
		if (errno == EINTR)
			errno = ETIMEDOUT;
	}
	alarm(0);
	if (mysignal(SIGALRM, old_sigfunc) == SIG_ERR)
		err_sys("mysignal error");
	return err;
}


/* 使用套接字选项SO_SNDTIMEO实现的connect定时设置 */
int connect_timeo1(int sockfd, const struct sockaddr* svaddr, socklen_t len, time_t nsec) {
	struct timeval tv;
	int err;

	tv.tv_sec = nsec;
	tv.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1)
		err_sys("setsockopt error");
	if ((err = connect(sockfd, svaddr, len)) == -1) {
		if (close(sockfd) == -1)
			err_sys("close error");
	}
	return err;
}
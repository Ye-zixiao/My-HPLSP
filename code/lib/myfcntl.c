#include "MyUNP.h"


/**
 * 对fd设置某一个文件描述符标志。实际操作中，只能设置O_CLOEXEC
 */
int set_fd(int fd, int nflag) {
	int flag;

	if ((flag = fcntl(fd, F_GETFD, 0)) == -1)
		return -1;
	flag |= nflag;
	return fcntl(fd, F_SETFD, flag);
}


/**
 * 对fd设置某一个文件状态标志。在Linux中，只能设置如下几个文件状态
 * 标志：O_APPEND, O_ASYNC, O_DIRECT, O_NOATIME, and O_NONBLOCK
 */
int set_fl(int fd, int nflag) {
	int flag;

	if ((flag = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	flag |= nflag;
	return fcntl(fd, F_SETFL, flag);
}


/**
 * 对fd清除某一个特定的文件描述符标志
 */
int clr_fd(int fd, int cflag) {
	int flag;
	
	if ((flag = fcntl(fd, F_GETFD, 0)) == -1)
		return -1;
	flag &= ~cflag;
	return fcntl(fd, F_SETFD, flag);
}


/**
 * 对fd清除某一个特定的文件状态标志
 */
int clr_fl(int fd, int cflag) {
	int flag;

	if ((flag = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	flag &= ~flag;
	return fcntl(fd, F_SETFL, flag);
}

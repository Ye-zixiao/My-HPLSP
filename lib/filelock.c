#include "MyUNP.h"


/* 申请文件记录锁，具体的操作主要取决于cmd和lock_type */
int lock_reg(int fd, int cmd, int lock_type,
	off_t offset, int whence, int len) {
	struct flock flock;

	flock.l_type = lock_type;
	flock.l_whence = whence;
	flock.l_start = offset;
	flock.l_len = len;
	return fcntl(fd, cmd, &flock);
}


/* 测试并设置文件记录锁 */
int lock_test(int fd, int cmd, int lock_type,
	off_t offset, int whence, int len) {
	struct flock flock;

	flock.l_type = lock_type;
	flock.l_whence = whence;
	flock.l_start = offset;
	flock.l_len = len;

	if (fcntl(fd, F_GETLK, &flock) == -1)
		err_sys("fcntl error");
	if (flock.l_type == F_UNLCK)
		return 0;
	return flock.l_pid;
}
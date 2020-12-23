#include "MyUNP.h"

static int read_cnt;
static char* read_ptr;
static char read_buf[MAXLINE];


/**
 * 若read_buf内部缓冲区中的数据都已被使用，则调用read一次性
 * 读入MAXLEN个字符，否则从缓冲区中取出一个字符
 */
static ssize_t myread(int fd, char* ptr) {
	if (read_cnt <= 0) {
again:
		if ((read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return -1;
		}
		else if (read_cnt == 0)
			return 0;
		read_ptr = read_buf;
	}

	*ptr = *read_ptr++;
	read_cnt--;
	return 1;
}


/**
 * 性能更好的readline()，可以通过自己提供内部缓冲区的方式
 * 来尽可能减少read系统调用，从而提高执行效率
 */
ssize_t readline(int fd, void* buf, size_t maxlen) {
	ssize_t nread, n;
	char ch, * ptr;

	ptr = buf;
	for (n = 1; n < maxlen; n++) {
		if ((nread = myread(fd, &ch)) == 1) {
			*ptr++ = ch;
			if (ch == '\n')
				break;
		}
		else if (nread == 0) {
			*ptr = 0;
			return n - 1;
		}
		else return -1;
	}

	*ptr = '\0';
	return n;
}


/**
 * 暴露readline()使用的内部缓冲区状态：缓冲区中剩余可读的字符
 * 数量以及指向剩余字符数组起始的指针。不过由于使用了静态数据，
 * 所以并不具有线程安全性和异常信号安全性
 */
ssize_t readlinebuf(void** cptrptr) {
	if (read_cnt)
		*cptrptr = read_ptr;
	return read_cnt;
}
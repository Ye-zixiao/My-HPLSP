#include "MyUNP.h"

/* 线程安全版本的readline函数 */

static pthread_key_t rl_key;
static pthread_once_t rl_once = PTHREAD_ONCE_INIT;


static void readline_once(void) {
	int err;
	if ((err = pthread_key_create(&rl_key, free)) != 0)
		err_exit(err, "pthread_key_create error");
}


struct Rline {
	int rl_cnt;			//当前缓冲区中剩余有效未读的数据量
	char* rl_bufptr;		//指向剩余有效数据缓冲区的起始地址
	char rl_buf[MAXLINE];
};



/**
 * 从readline私有缓冲区读取一个字符，若为空，则先调用read输入数据以填充
 * @param	fd			文件描述符
 * @param	rlineptr		指向readline私有缓冲区的结构指针
 * @param	ptr			指向当前使用到的字符
 * @return	成功返回1，失败返回-1，读到文件结束标志返回0
 */
static ssize_t my_read(int fd, struct Rline* rlineptr, char* ptr) {
	if (rlineptr->rl_cnt <= 0) {
	again:
		if ((rlineptr->rl_cnt = read(fd, rlineptr->rl_buf, MAXLINE)) == -1) {
			if (errno == EINTR)goto again;
			return -1;
		}
		else if (rlineptr->rl_cnt == 0)
			return 0;
		rlineptr->rl_bufptr = rlineptr->rl_buf;
	}

	rlineptr->rl_cnt--;
	*ptr = *rlineptr->rl_bufptr++;
	return 1;
}


/* readline的线程安全版本 */
ssize_t readline_r(int fd, void* buf, size_t maxlen) {
	struct Rline* rlineptr;
	size_t n, rc;
	char c, * ptr;
	int err;

	/* 确保线程私有数据相关的键只被调用一次，并创建出线程私有数据 */
	if ((err = pthread_once(&rl_once, readline_once)) != 0)
		err_exit(err, "pthread_once error");
	if ((rlineptr = pthread_getspecific(rl_key)) == NULL) {
		if ((rlineptr = calloc(1, sizeof(struct Rline))) == NULL)
			err_sys("calloc error");
		if ((err = pthread_setspecific(rl_key, rlineptr)) != 0)
			err_exit(err, "pthread_setspecific error");
	}

	/* 从readline私有的缓冲区中读取一个文本行 */
	ptr = buf;
	for (n = 1; n < maxlen; ++n) {
		if ((rc = my_read(fd, rlineptr, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')break;
		}
		else if (rc == 0) {
			*ptr = 0;
			return n - 1;
		}
		else return -1;
	}

	*ptr = 0;
	return n;
}
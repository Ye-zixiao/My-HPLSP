#include "MyUNP.h"


/**
 * 从指定文件描述符中读取一行数据，若没有遇到换行符则该函数会自动
 * 对最后一个字符进行填充'\n'。不过这个函数执行效率非常低
 */
ssize_t readline1(int fd, void* buf, size_t maxlen) {
	ssize_t nread, n;
	char c, * ptr;

	ptr = buf;
	/* i从1计数是为了预留一个字符以在buf不能容纳的时候
		在最后面放置一个'\n' */
	for (n = 1; n < maxlen; ++n) {
	again:
		if ((nread = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;
		}
		//遇到文件结束符EOF
		else if (nread == 0) {
			*ptr = 0;
			return n - 1;
		}
		else {
			if (errno == EINTR)
				goto again;
			return -1;
		}
	}
	*ptr = '\0';
	return n;
}
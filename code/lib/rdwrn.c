#include "MyUNP.h"

#ifndef MSG_WAITALL
/**
 * 从指定套接字读取指定字节数的数据
 */
ssize_t readn(int fd, void* buf, size_t nbytes) {
	size_t nleft;
	ssize_t nread;
	char* ptr;

	ptr = buf;
	nleft = nbytes;
	while (nleft > 0) {
		if ((nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;
			else return -1;
		}
		else if (nread == 0)
			break;

		nleft -= nread;
		ptr += nread;
	}
	return nbytes - nleft;
}


/**
 * 向指定套接字写入指定字节数的数据
 */
ssize_t writen(int fd, const void* buf, size_t nbytes) {
	size_t nleft;
	ssize_t nwrite;
	const char* ptr;

	ptr = buf;
	nleft = nbytes;
	while (nleft > 0) {
		if ((nwrite = write(fd, ptr, nleft)) <= 0) {
			if (nwrite < 0 && errno == EINTR)
				nwrite = 0;
			else return -1;
		}

		nleft -= nwrite;
		ptr += nwrite;
	}
	return nbytes - nleft;
}


#endif
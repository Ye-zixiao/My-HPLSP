#include "MyUNP.h"


/** 
 * 等待指定套接字描述符可读nsec秒，若返回值大于0，则表示
 * 该文件描述符可读，此时即可调用read/recv等函数
 */
int readable_timeo(int sockfd, time_t nsec) {
	struct timeval timebuf;
	fd_set rset;

	FD_ZERO(&rset);
	FD_SET(sockfd, &rset);
	timebuf.tv_sec = nsec;
	timebuf.tv_usec = 0;
	return select(sockfd + 1, &rset, NULL, NULL, &timebuf);
}


/**
 * 等待指定套接字描述符可写nsec秒，若返回值大于0，则表示
 * 该文件描述符可写，此时即可调用write/send等函数
 */
int writeable_timeo(int sockfd, time_t nsec) {
	struct timeval timebuf;
	fd_set wset;

	FD_ZERO(&wset);
	FD_SET(sockfd, &wset);
	timebuf.tv_sec = nsec;
	timebuf.tv_usec = 0;
	return select(sockfd + 1, NULL, &wset, NULL, &timebuf);
}
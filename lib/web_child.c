#include "MyUNP.h"

#define MAXN 16384


/* 接收来自客户的数据请求，并返回给它指定数量的数据 */
void web_child(int sockfd) {
	char line[MAXLINE], result[MAXN];
	int ntowrite; ssize_t nread;

	for (;;) {
		if ((nread = readline(sockfd, line, MAXLINE)) == -1)
			err_sys("readline error");
		else if (nread == 0)return;

		ntowrite = atoi(line);
		if (ntowrite <= 0 || ntowrite > MAXN)
			err_quit("client request for %d bytes", ntowrite);
		if (writen(sockfd, result, ntowrite) != ntowrite)
			err_sys("writen error");
	}
}


/* web_child的线程安全版本 */
void web_child_r(int sockfd) {
	char line[MAXLINE], result[MAXN];
	int ntowrite; ssize_t nread;

	for (;;) {
		if ((nread = readline_r(sockfd, line, MAXLINE)) == -1)
			err_sys("readline_r error");
		else if (nread == 0) return;

		ntowrite = atoi(line);
		if (ntowrite <= 0 || ntowrite > MAXN)
			err_quit("client request for %d bytes", ntowrite);
		if (writen(sockfd, result, ntowrite) != ntowrite)
			err_sys("writen error");
	}
}


/* web_child的另一个线程安全版本，其中调用的readline1内部是没有自己的
	缓冲区的，而是直接一个一个字节的调用read。仅适合读取短字符串使用 */
void web_child_r1(int sockfd) {
	char line[MAXLINE], result[MAXN];
	int ntowrite; ssize_t nread;

	for (;;) {
		if ((nread = readline1(sockfd, line, MAXLINE)) == -1)
			err_sys("readline1 error");
		else if (nread == 0) return;

		ntowrite = atoi(line);
		if (ntowrite <= 0 || ntowrite > MAXN)
			err_quit("client request for %d bytes", ntowrite);
		if (writen(sockfd, result, ntowrite) != ntowrite)
			err_sys("writen error");
	}
}
#include "MyUNP.h"
#include <ctype.h>


static void upper(char* buf, int n) {
	for (int i = 0; i < n; ++i) {
		if (buf[i] == 0)break;
		buf[i] = toupper(buf[i]);
	}
}


/**
 * 从因特网中接收到UDP数据报并进行回射
 */
void dg_echo(int sockfd, struct sockaddr* cliaddr, socklen_t clilen) {
	char buf[MAXLINE];
	ssize_t nrecv;
	socklen_t len;

	for (;;) {
		len = clilen;
		if ((nrecv = recvfrom(sockfd, buf, MAXLINE, 0, cliaddr, &len)) == -1)
			err_sys("recvfrom error");
		upper(buf, sizeof(buf));
		if (sendto(sockfd, buf, nrecv, 0, cliaddr, len) == -1)
			err_sys("sendto error");
	}
}


static int count;

static void recvfrom_int(int signo) {
	printf("\nreceived %d datagrams\n", count);
	exit(EXIT_SUCCESS);
}

void dg_echox(int sockfd, struct sockaddr* cliaddr, socklen_t clilen) {
	char message[MAXLINE];
	socklen_t len;
	int n;

	n = 10 * 1024;
	if(setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &n, sizeof(int)) == -1)
		err_sys("setsockopt error");
	if (mysignal(SIGINT, recvfrom_int) == SIG_ERR)
		err_sys("mysignal error");
	for (;;) {
		len = clilen;
		if (recvfrom(sockfd, message, MAXLINE, 0, cliaddr, &len) == -1)
			err_sys("recvfrom error");
//		printf("recvfrom success\n");
		++count;
	}
}

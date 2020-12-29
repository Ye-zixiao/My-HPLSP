#include "MyUNP.h"

/* 使用SIGURG信号来通知带外数据的到来 */

static int connfd;

void sig_urg(int signo) {
	int nrecv, errno_save = errno;
	char buf[MBUFSIZE];

	if ((nrecv = recv(connfd, buf, sizeof(buf) - 1, MSG_OOB)) == -1)
		err_sys("recv error");
	buf[nrecv] = 0;
	printf("got %d bytes of oob data '%s'\n", nrecv, buf);
	errno = errno_save;
}


int main(int argc, char* argv[])
{
	char buf[BUFSIZE];
	int listenfd;
	ssize_t nrecv;

	if (argc != 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	if (mysignal(SIGURG, sig_urg) == SIG_ERR)
		err_sys("mysignal error");
	listenfd = tcp_listen(NULL, argv[1], NULL);

	if ((connfd = accept(listenfd, NULL, NULL)) == -1)
		err_sys("accept error");
	if (fcntl(connfd, F_SETOWN, getpid()) == -1)
		err_sys("fcntl error");
	while ((nrecv = read(connfd, buf, sizeof(buf) - 1)) > 0) {
		buf[nrecv] = 0;
		printf("got %ld bytes of normal data: %s\n", nrecv, buf);
	}
	if (close(connfd) == -1)
		err_sys("close error");
	exit(EXIT_SUCCESS);
}
#include "MyUNP.h"

/**
 * 统一事件源管理：主进程使用I/O复用的方式统一管理
 * I/O事件、异步信号事件
 */

#define MAX_EVENT_NUMS 1024
#define TRUE 1
#define FALSE 0
int pipefds[2];


void addfd(int epfd, int fd) {
	struct epoll_event event;

	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1)
		err_sys("epoll_ctl error");
	if (set_fl(fd, O_NONBLOCK) == -1)
		err_sys("set_fl error");
}


/* 进程接收到信号之后,信号处理程序仅仅只是将这个信号值发送到指定
	管道之中,让主进程通过I/O复用的方式来统一管理I/O事件、信号   */
void sig_handler(int signo) {
	int errno_save = errno;
	if (write(pipefds[1], (char*)&signo, sizeof(int)) != sizeof(int))
		err_sys("write error");
	errno = errno_save;
}


int main(int argc, char* argv[])
{
	int epfd, listenfd, connfd, sockfd, nret, stop, temp;
	struct epoll_event events[MAX_EVENT_NUMS];
	ssize_t nrecv;

	if (argc != 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	listenfd = tcp_listen(NULL, argv[1], NULL);
	if ((epfd = epoll_create(10)) == -1)
		err_sys("epoll_create error");
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipefds) == -1)
		err_sys("socketpair error");
	if (set_fl(pipefds[1], O_NONBLOCK) == -1)
		err_sys("set_fl error");
	addfd(epfd, listenfd);
	addfd(epfd, pipefds[0]);

	if (mysignal(SIGHUP, sig_handler) == SIG_ERR)
		err_sys("mysignal error");
	if (mysignal(SIGINT, sig_handler) == SIG_ERR)
		err_sys("mysignal error");
	if (mysignal(SIGTERM, sig_handler) == SIG_ERR)
		err_sys("mysignal error");
	if (mysignal(SIGCHLD, sig_handler) == SIG_ERR)
		err_sys("mysignal error");

	for (stop = FALSE; !stop;) {
		if ((nret = epoll_wait(epfd, events, MAX_EVENT_NUMS, -1)) == -1) {
			if (errno == EINTR) continue;
			err_sys("epoll_wait error");
		}

		for (int i = 0; i < nret; ++i) {
			sockfd = events[i].data.fd;

			if (sockfd == listenfd) {
				if ((connfd = accept(listenfd, NULL, NULL)) == -1)
					err_sys("accept error");
				printf("get new connection\n");
				addfd(epfd, connfd);
			}
			else if (sockfd == pipefds[0] && (events[i].events & EPOLLIN)) {
				if ((nrecv = read(pipefds[0], &temp, sizeof(int))) == -1)
					err_sys("read error");
				else if (nrecv == 0) continue;
				switch (temp) {
				case SIGHUP:case SIGCHLD: continue;
				case SIGTERM:case SIGINT: stop = TRUE;
				}
			}
			/* else ignore */
		}
	}

	printf("\nserver closing\n");
	exit(EXIT_SUCCESS);
}
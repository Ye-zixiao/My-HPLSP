#include "MyUNP.h"

/**
 * 测试epoll机制的EPOLLONESHOT特性
 */

#define TRUE	  1
#define FALSE 0
#define MAX_EVENT_NUMS 1024

struct fds {
	int epfd, sockfd;
};


void addfd(int epfd, int fd, int oneshot) {
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	if (oneshot) event.events |= EPOLLONESHOT;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1)
		err_sys("epoll_ctl error");
	if (set_fl(fd, O_NONBLOCK) == -1)
		err_sys("set_fl error");
}


/* 为指定套接字描述符在epoll内核时间表中重新设置ONESHOT事件 */
void reset_oneshot(int epfd, int sockfd) {
	struct epoll_event event;
	event.data.fd = sockfd;
	event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	if (epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event) == -1)
		err_sys("epoll_ctl error");
}


void* thread(void* arg) {
	int sockfd = ((struct fds*)arg)->sockfd,
		epfd = ((struct fds*)arg)->epfd;
	char buf[BUFSIZE];
	ssize_t nread;

	printf("start new thread to receive data on fd: %d\n", sockfd);
	while ((nread = read(sockfd, buf, BUFSIZE - 1)) > 0) {
		buf[nread] = 0;
		printf("get content: %s\n", buf);
		sleep(5);
	}
	if (nread == 0) {
		if (close(sockfd) == -1)
			err_sys("close error");
		printf("foreiner closed the connection\n");
	}
	else if (nread == -1 && errno == EWOULDBLOCK) {
		reset_oneshot(epfd, sockfd);
		printf("read later\n");
	}
	else if (nread == -1 && errno == ECONNRESET) {
		if (close(sockfd) == -1)
			err_sys("close error");
		printf("connection reset\n");
	}
	else err_sys("read error");
	printf("end thread receiving data on fd: %d\n\n", sockfd);
	pthread_exit((void*)NULL);
}


int main(int argc, char* argv[])
{
	int epfd, listenfd, connfd, sockfd, nret, err;
	struct epoll_event events[MAX_EVENT_NUMS];
	pthread_t tid;

	if (argc == 2)
		listenfd = tcp_listen(NULL, argv[1], NULL);
	else if (argc == 3)
		listenfd = tcp_listen(argv[1], argv[2], NULL);
	else
		err_quit("usage: %s [host/ip] <serv/port>", basename(argv[0]));

	if ((epfd = epoll_create(10)) == -1)
		err_sys("epoll_create error");
	addfd(epfd, listenfd, FALSE);

	for (;;) {
		if ((nret = epoll_wait(epfd, events, MAX_EVENT_NUMS, -1)) == -1)
			err_sys("epoll_wait error");

		for (int i = 0; i < nret; ++i) {
			sockfd = events[i].data.fd;

			if (sockfd == listenfd) {
				if ((connfd = accept(listenfd, NULL, NULL)) == -1)
					err_sys("accept error");
				addfd(epfd, connfd, TRUE);
			}
			else if (events[i].events & EPOLLIN) {
				/* 当该设置了EPOLLONESHOT的描述符被通知了一次之后，内核中的epoll
					感兴趣描述符列表就会将这个描述符置为无效，在下一次调用epoll_wait
					的时候epoll是不会通知应用进程关于该描述符上发生的一切事件。除非
					工作线程在结束时为该描述符重新设置感兴趣事件  */
				struct fds fds = { epfd,sockfd };
				if ((err = pthread_create_detached(&tid, thread, (void*)&fds)) != 0)
					err_exit(err, "pthread_create_detached error");
			}
			else printf("something else happened\n");
		}
	}
}
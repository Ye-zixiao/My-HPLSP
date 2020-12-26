#include "MyUNP.h"

#define TRUE	  1
#define FALSE 0
#define MAX_EVENT_NUMS 1024


void addfd(int epfd, int fd, int enable_et) {
	struct epoll_event event;
	event.data.fd = fd;
	event.events = enable_et ? EPOLLIN | EPOLLET : EPOLLIN;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1)
		err_sys("epoll_ctl error");
	if (set_fl(fd, O_NONBLOCK) == -1)
		err_sys("set_fl error");
}


void level_trigger(struct epoll_event* ep, int nums, int epfd, int listenfd) {
	struct sockaddr_storage ss;
	int sockfd, connfd;
	char buf[BUFSIZE];
	socklen_t len;
	ssize_t nrecv;

	for (int i = 0; i < nums; ++i) {
		sockfd = ep[i].data.fd;

		if (sockfd == listenfd) {
			len = sizeof(ss);
			if ((connfd = accept(listenfd, (struct sockaddr*)&ss, &len)) == -1)
				err_sys("accept error");
			printf("new connection from %s\n", sock_ntop((const struct sockaddr*)&ss, len));

			addfd(epfd, connfd, FALSE);
		}
		else if (ep[i].events & EPOLLIN) {
			printf("event trigger once\n");
			if ((nrecv = read(sockfd, buf, sizeof(buf) - 1)) == -1)
				err_sys("read error");
			else if (nrecv == 0) {
				if (epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL) == -1)
					err_sys("epoll_ctl error");
				if (close(sockfd) == -1)
					err_sys("close error");
				continue;
			}
			buf[nrecv] = 0;
			printf("get %ld bytes of content: %s\n", nrecv, buf);
		}
		else printf("something else happened\n");
	}
}


void edge_trigger(struct epoll_event* ep, int nums, int epfd, int listenfd) {
	struct sockaddr_storage ss;
	int sockfd, connfd;
	char buf[BUFSIZE];
	socklen_t len;
	ssize_t nrecv;

	for (int i = 0; i < nums; ++i) {
		sockfd = ep[i].data.fd;

		if (sockfd == listenfd) {
			len = sizeof(ss);
			if ((connfd = accept(listenfd, (struct sockaddr*)&ss, &len)) == -1)
				err_sys("accept error");
			printf("new connection from %s\n", sock_ntop((const struct sockaddr*)&ss, len));

			addfd(epfd, connfd, TRUE);
		}
		else if (ep[i].events & EPOLLIN) {
			printf("event trigger once\n");
			while ((nrecv = read(sockfd, buf, sizeof(buf) - 1)) > 0) {
				buf[nrecv] = 0;
				printf("get %ld bytes of content: %s\n", nrecv, buf);
			}
			if (nrecv == 0) {
				if (epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL) == -1)
					err_sys("epoll_ctl error");
				if (close(sockfd) == -1)
					err_sys("close error");
			}
			else if (nrecv == -1 && errno == EWOULDBLOCK) {
				printf("read later\n");
			}
			else err_sys("read error");
		}
		else printf("something else happened\n");
	}
}



int main(int argc, char* argv[])
{
	struct epoll_event events[MAX_EVENT_NUMS];
	int listenfd, epfd, ret;

	if (argc == 2)
		listenfd = tcp_listen(NULL, argv[1], NULL);
	else if (argc == 3)
		listenfd = tcp_listen(argv[1], argv[2], NULL);
	else
		err_quit("usage: %s [host/ip] <serv/port>", basename(argv[0]));

	if ((epfd = epoll_create(10)) == -1)
		err_sys("epoll_create error");
	addfd(epfd, listenfd, TRUE);

	for (;;) {
		if ((ret = epoll_wait(epfd, events, MAX_EVENT_NUMS, -1)) == -1)
			err_sys("epoll_wait error");

		//level_trigger(events, ret, epfd, listenfd);
		edge_trigger(events, ret, epfd, listenfd);
	}
}
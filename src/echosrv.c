#include "MyUNP.h"

/**
 * 同时支持UDP和TCP服务的回射服务器
 */


#define MAX_EVENT_NUMS 1024

void addfd(int epfd, int fd) {
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1)
		err_sys("addfd error");
	if (set_fl(fd, O_NONBLOCK) == -1)
		err_sys("set_fl error");
}


int main(int argc, char* argv[])
{
	int nret, epfd, udpfd, sockfd, listenfd, connfd;
	struct epoll_event events[MAX_EVENT_NUMS];
	struct sockaddr_in cliaddr;
	char buf[BUFSIZE];
	socklen_t clilen;
	ssize_t nrecv;

	if (argc != 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	listenfd = tcp_listen(NULL, argv[1], NULL);
	udpfd = udp_server(NULL, argv[1], NULL);
	if ((epfd = epoll_create(10)) == -1)
		err_sys("epoll_create error");
	addfd(epfd, listenfd);
	addfd(epfd, udpfd);

	for (;;) {
		if ((nret = epoll_wait(epfd, events, MAX_EVENT_NUMS, -1)) == -1)
			err_sys("epoll_wait error");

		for (int i = 0; i < nret; ++i) {
			sockfd = events[i].data.fd;

			if (sockfd == listenfd) {
				clilen = sizeof(cliaddr);
				if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1)
					err_sys("accept error");
				printf("%s: new connection from %s\n", currtime("%T"),
					sock_ntop((const struct sockaddr*)&cliaddr, clilen));
				addfd(epfd, connfd);
			}
			else if (sockfd == udpfd) {
				clilen = sizeof(cliaddr);
				if ((nrecv = recvfrom(udpfd, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&cliaddr, &clilen)) == -1)
					err_sys("recvfrom error");
				printf("%s: received new datagram from %s\n", currtime("%T"),
					sock_ntop((const struct sockaddr*)&cliaddr, clilen));

				buf[nrecv] = 0;
				if (sendto(udpfd, buf, strlen(buf), 0, (struct sockaddr*)&cliaddr, clilen) == -1)
					err_sys("sendto error");
			}
			else if (events[i].events & EPOLLIN) {
				while ((nrecv = read(sockfd, buf, sizeof(buf) - 1)) > 0) {
					buf[nrecv] = 0;
					if (write(sockfd, buf, strlen(buf)) != strlen(buf))
						err_sys("write error");
				}
				if (nrecv == 0) {
					if (epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL) == -1)
						err_sys("epoll_ctl error");
					if (close(sockfd) == -1)
						err_sys("close error");
				}
				else if (nrecv == -1 && errno != EWOULDBLOCK)
					err_sys("read error");
			}
			else printf("something else happened\n");
		}
	}
}
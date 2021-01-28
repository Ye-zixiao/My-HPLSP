#include "MyUNP.h"

static int stop = 0;
static const char* request =
	"GET http://localhost/index.html HTTP/1.1\r\nHost: winhost\r\nConnection: keep-alive\r\n\r\n";

void addfd(int epfd, int fd) {
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLOUT | EPOLLET | EPOLLERR;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

void close_conn(int epfd, int sockfd) {
	err_msg("fd %d is closing\n", sockfd);
	epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL);
	close(sockfd);
}

int read_once(int sockfd, char* buffer, size_t len) {
	ssize_t nread = read(sockfd, buffer, len);
	if (nread <= 0) {
		printf("get nread == %ld from fd %d\n", nread, sockfd);
		return -1;
	}
	printf("get %ld bytes data from fd %d\n", nread, sockfd);
	return 0;
}

void start_conn(int epfd, int num, const char* host, const char* serv) {
	int sockfd;
	for (int i = 0; i < num; ++i) {
		if ((sockfd = tcp_connect(host, serv)) == -1) {
			err_ret("tcp_connect error");
			continue;
		}
		printf("build connection %d\n", i);
		addfd(epfd, sockfd);
	}
}

static void sig_int(int signo) { stop = 1; }


int main(int argc, char* argv[])
{
	struct epoll_event events[10000];
	struct epoll_event event;
	int sockfd, epfd, nret;
	char buf[2024];

	if (argc != 4)
		err_quit("usage: %s <host/ip> <serv/port> <conn_num>", basename(argv[0]));
	mysignal1(SIGINT, sig_int);
	if ((epfd = epoll_create(100)) == -1)
		err_sys("epoll_create error");
	start_conn(epfd, atoi(argv[3]), argv[1], argv[2]);

	while (!stop) {
		if ((nret = epoll_wait(epfd, events, 10000, -1)) == -1) {
			if (errno == EINTR) continue;
			err_sys("epoll_wait error");
		}

		for (int i = 0; i < nret; ++i) {
			sockfd = events[i].data.fd;

			if (events[i].events & EPOLLIN) {
				if (read_once(sockfd, buf, sizeof(buf) - 1) == -1) {
					close_conn(epfd, sockfd);
					printf("1------\n");
				}
				event.data.fd = sockfd;
				event.events = EPOLLOUT | EPOLLET | EPOLLERR;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
			}
			else if (events[i].events & EPOLLOUT) {
				if (writen(sockfd, request, strlen(request)) == -1) {
					close_conn(epfd, sockfd);
					printf("2-----\n");
				}
				//if (write(sockfd, request, strlen(request)) == -1) {
				//	close_conn(epfd, sockfd);
				//	printf("2-----\n");
				//}
				event.data.fd = sockfd;
				event.events = EPOLLIN | EPOLLERR | EPOLLET;
				epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &event);
			}
			else if (events[i].events & EPOLLERR) {
				close_conn(epfd, sockfd);
				printf("3-------\n");
			}
		}
	}
	exit(EXIT_SUCCESS);
}

#include "MyUNP.h"


/* 将指定描述符添加到epoll内核事件表中，默认的事件为EPOLLIN
	| EPOLLET，开启边沿触发，并将描述符设置到非阻塞模式  */
void add2epoll(int epfd, int fd) {
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1)
		err_sys("epoll_ctl error");
	if (set_fl(fd, O_NONBLOCK) == -1)
		err_sys("set_fl error");
}


void rmfdepoll(int epfd, int fd) {
	if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
		err_sys("epoll_ctl error");
	if (close(fd) == -1)
		err_sys("close error");
}
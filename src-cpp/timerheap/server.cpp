extern "C" {
#include "MyUNP.h" 
}
#include "TimerHeap.h"
#include <iostream>

#define MAX_EVENTS_NUM 1024
#define FD_LIMITS 65536
#define TIMEOUT 15

static Client_Data users[FD_LIMITS];
static TimerHeap timer_heap;
static int epfd;


void callback_func(Client_Data* user_data);
void next_clock(time_t& timeout, const time_t& start);


int main(int argc, char* argv[])
{
	int listenfd, sockfd, connfd, nret;
	epoll_event events[MAX_EVENTS_NUM];
	sockaddr_in cliaddr;
	socklen_t clilen;
	ssize_t nread;

	if (argc != 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	listenfd = tcp_listen(NULL, argv[1], NULL);
	if ((epfd = epoll_create(10)) == -1)
		err_sys("epoll_create error");
	add2epoll(epfd, listenfd);

	time_t start, timeout = -1;
	for (;;) {
		time(&start);
		if ((nret = epoll_wait(epfd, events, MAX_EVENTS_NUM, timeout)) == -1) {
			if (errno == EINTR) continue;
			err_sys("epoll_wait error");
		}
		else if (nret == 0) {
#ifdef DEBUG
			std::cout << currtime("%T") << ": ";
#endif // DEBUG
			timer_heap.tick();

			if (timer_heap.empty()) timeout = -1;
			else timeout = (timer_heap.top_timer()->expire - time(NULL)) * 1000;
			continue;
		}

		for (int i = 0; i < nret; ++i) {
			sockfd = events[i].data.fd;

			if (sockfd == listenfd) {
				clilen = sizeof(cliaddr);
				if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1)
					err_sys("accept error");
				std::cout << currtime("%T") << ": new connection from " 
					<< sock_ntop((const struct sockaddr*)&cliaddr, clilen) << std::endl;

				add2epoll(epfd, connfd);
				users[connfd].cliaddr = cliaddr;
				users[connfd].sockfd = connfd;
				HTimer* timer = new HTimer{
					callback_func,&users[connfd],TIMEOUT + time(NULL) };
				users[connfd].timer = timer;
				timer_heap.add_timer(timer);
			}
			else if (events[i].events & EPOLLIN) {
				HTimer* timer = users[sockfd].timer;

				while ((nread = read(sockfd, users[sockfd].buf, BUFFER_SIZE - 1)) > 0) {
					users[sockfd].buf[nread] = 0;
					std::cout << currtime("%T") << ": get " << nread 
						<< " bytes from fd " << sockfd << std::endl;
					if (timer) {
						timer_heap.del_timer(timer);
						HTimer* newtimer = new HTimer{
							callback_func,&users[sockfd],TIMEOUT + time(NULL) };
						users[sockfd].timer = newtimer;
						timer_heap.add_timer(newtimer);
						timer = newtimer;
					}
				}
				if (nread == 0 || (nread == -1 && errno != EWOULDBLOCK)) {
					callback_func(&users[sockfd]);
					if (timer) timer_heap.del_timer(timer);
				}
			}

			next_clock(timeout, start);
#ifdef DEBUG
			std::cout << timeout << std::endl;
#endif // DEBUG
		}
	}
}


void callback_func(Client_Data* user_data) {
	int sockfd = user_data->sockfd;
	if (epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, NULL) == -1)
		err_sys("epoll_ctl error");
	if (close(sockfd) == -1)
		err_sys("close error");
	std::cout << currtime("%T") << ": close fd " << sockfd << std::endl;
}


void next_clock(time_t& timeout, const time_t& start) {
	//首先会去除时间堆中头部中无效的定时器
	while (!timer_heap.empty() && !timer_heap.top_timer()->callback)
		timer_heap.pop_timer();

	/* 若上一次的定时是-1，则在时间堆中有定时器的情况下选择一个
		新的定时器 */
	if (timeout == -1) {
		if (!timer_heap.empty())
			timeout = (timer_heap.top_timer()->expire - time(NULL)) * 1000;
	}
	/* 若上次的定时非-1，则若上一次定时仍有效则继续该定时，
		否则从时间堆中找新的定时器 */
	else {
		timeout -= (time(NULL) - start) * 1000;
		if (timeout <= 0) {
			if (!timer_heap.empty())
				timeout = (timer_heap.top_timer()->expire - time(NULL)) * 1000;
			else timeout = -1;
		}
	}
}

extern "C" {
#include "MyUNP.h"
}
#include "lst_timer.h"

#define MAX_EVENT_NUMS 1024
#define FD_LIMIT 65535
#define TIMESLOT 5

static int epfd, pipefds[2];
static sort_time_lst timer_lst;


void addfd(int epfd, int fd) {
	epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event) == -1)
		err_sys("epoll_ctl error");
	if (set_fl(fd, O_NONBLOCK) == -1)
		err_sys("set_fl error");
}


void sig_handler(int signo) {
	int errno_save = errno;
	//if (write(pipefds[1], &signo, sizeof(int)) != sizeof(int))//向管道写入1字节的信号编码
	//	err_sys("write error");
	if (write(pipefds[1], &signo, 1) != 1)
		err_sys("write error");
	errno = errno_save;
}


void timer_handler() {
	timer_lst.tick();
	alarm(TIMESLOT);
}


/* 回调函数，用来关闭连接并将相关描述符从epoll内核事件表中移除 */
void cb_func(client_data* user_data) {
	if (epoll_ctl(epfd, EPOLL_CTL_DEL, user_data->sockfd, NULL) == -1)
		err_sys("epoll_ctl error");
	if (close(user_data->sockfd) == -1)
		err_sys("close error");
	std::cout << "close fd " << user_data->sockfd << std::endl;
}


int main(int argc, char* argv[])
{
	int sockfd, connfd, listenfd, nret;
	epoll_event events[MAX_EVENT_NUMS];
	sockaddr_in cliaddr;
	client_data* users;
	bool stop, timeout;
	char signals[1024];
	ssize_t nrecv;

	if (argc != 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));
	
	listenfd = tcp_listen(NULL, argv[1], NULL);
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, pipefds) == -1)
		err_sys("socketpair error");
	if ((epfd = epoll_create(10)) == -1)
		err_sys("epoll_create error");
	if (set_fl(pipefds[1], O_NONBLOCK) == -1)
		err_sys("set_fl error");
	addfd(epfd, pipefds[0]);
	addfd(epfd, listenfd);

	stop = timeout = false;
	users = new client_data[FD_LIMIT];
	if (mysignal1(SIGALRM, sig_handler) == SIG_ERR)
		err_sys("mysignal error");
	if (mysignal1(SIGTERM, sig_handler) == SIG_ERR)
		err_sys("mysignal error");
	alarm(TIMESLOT);

	for (; !stop;) {
		if ((nret = epoll_wait(epfd, events, MAX_EVENT_NUMS, -1)) == -1) {
			if (errno == EINTR) continue;
			err_sys("epoll_wait error");
		}

		for (int i = 0; i < nret; ++i) {
			sockfd = events[i].data.fd;

			/* 有新的连接请求到来 */
			if (sockfd == listenfd) {
				socklen_t clilen = sizeof(sockaddr_in);
				if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1)
					err_sys("accept error");
				std::cout << currtime("%T") << ": new connection from "
					<< sock_ntop((const struct sockaddr*)&cliaddr, clilen)<< std::endl;

				//更新用户信息users并添加定时器到定时器链表中
				addfd(epfd, connfd);
				users[connfd].addr = cliaddr;
				users[connfd].sockfd = connfd;
				util_timer* timer = new util_timer;
				timer->user_data = &users[connfd];
				timer->cb_func = cb_func;
				timer->expire = time(NULL) + 3 * TIMESLOT;
				users[connfd].timer = timer;
				timer_lst.add_timer(timer);
			}
			/* unix域套接字管道中接收到新消息 */
			else if (sockfd == pipefds[0] && (events[i].events & EPOLLIN)) {
				if ((nrecv = read(pipefds[0], signals, sizeof(signals))) == -1)
					err_sys("read error");
				else if (nrecv == 0) continue;

				for (int j = 0; j < nret; j++) {
					switch (signals[j]) {
					case SIGALRM: timeout = true; break;
					case SIGTERM: stop = true;
					}
				}
			}
			/* 网络套接字接收到新的数据，说明相关连接是出于活动状态的 */
			else if (events[i].events & EPOLLIN) {
				util_timer* timer = users[sockfd].timer;

				while ((nrecv = read(sockfd, users[sockfd].buf, BUFFER_SIZE - 1)) > 0) {
					users[sockfd].buf[nrecv] = 0;
					std::cout << "get " << nrecv << " bytes of client data " << users[sockfd].buf
						<< " from " << sockfd << std::endl;
					if (timer) {
						timer->expire = time(NULL) + 3 * TIMESLOT;
						std::cout << "adjust timer once" << std::endl;
						timer_lst.adjust_timer(timer);
					}
				}
				if (nrecv == 0 || (nrecv == -1 && errno != EWOULDBLOCK)) {
					cb_func(&users[sockfd]);
					if (timer) timer_lst.del_timer(timer);
				}
			}

			if (timeout) {
				timer_handler();
				timeout = false;
			}
		}
	}

	exit(EXIT_SUCCESS);
}
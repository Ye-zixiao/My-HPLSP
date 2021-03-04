#include "http_conn.h"
#include "threadpool.h"

#define MAX_FD 65536
#define MAX_EVENT_NUM 10240

static bool stop = false;

extern int addfd(int, int, bool);
extern int removefd(int, int);

void show_error(int connfd, const char* info) {
	printf("%s", info);
	write(connfd, info, strlen(info));
	close(connfd);
}

static void sig_handler(int signo) { stop = true; }


int main(int argc, char* argv[])
{
	if (argc < 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	mysignal1(SIGPIPE, SIG_IGN);
	mysignal1(SIGINT, sig_handler);
	mysignal1(SIGQUIT, sig_handler);
	mysignal1(SIGTERM, sig_handler);

	//创建工作线程池
	threadpool<http_conn>* pool = nullptr;
	try { pool = new threadpool<http_conn>(8,12000); }
	catch (...) { exit(EXIT_FAILURE); }
	//创建用户信息数据表
	http_conn* users = new http_conn[MAX_FD];

	int listenfd = tcp_listen(nullptr, argv[1], nullptr);
	struct linger lingerbuf{1, 0};
	setsockopt(listenfd, SOL_SOCKET, SOL_SOCKET, &lingerbuf, sizeof(lingerbuf));
	struct epoll_event events[MAX_EVENT_NUM];
	int epfd = epoll_create(5);
	addfd(epfd, listenfd, false);
	http_conn::m_epfd = epfd;

	while (!stop) {
		int nret = epoll_wait(epfd, events, MAX_EVENT_NUM, -1);
		if (nret == -1 && errno != EINTR) {
			err_msg("epoll_wait error");
			break;
		}

		for (int i = 0; i < nret; ++i) {
			int sockfd = events[i].data.fd;

			if (sockfd == listenfd) {
				struct sockaddr_in cliaddr;
				socklen_t clilen = sizeof(cliaddr);
				int connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen);
				if (connfd == -1) {
					err_msg("accept error");
					continue;
				}
				std::cout << currtime("%T") << ": new connection from " <<
					sock_ntop((struct sockaddr*)&cliaddr, clilen) << std::endl;

				if (http_conn::m_user_cnt >= MAX_FD) {
					show_error(connfd, "Internal server busy");
					continue;
				}
				users[connfd].init(connfd, cliaddr);
			}
			else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
				users[sockfd].close_conn();
			else if (events[i].events & EPOLLIN) {
				if (users[sockfd].read())
					pool->append(users + sockfd);
				else users[sockfd].close_conn();
			}
			else if (events[i].events & EPOLLOUT) {
				if (!users[sockfd].write())
					users[sockfd].close_conn();
			}
		}
	}

	std::cout << "\nserver is closing..." << std::endl;
	close(epfd);
	close(listenfd);
	delete[] users;
	delete pool;
	exit(EXIT_SUCCESS);
}

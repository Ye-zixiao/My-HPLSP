#ifndef PROCESSPOOL_H_
#define PROCESSPOOL_H_

#include "MyUNP.h"
#include <iostream>

static int sig_pipefd[2];

//进程信息
struct Process_Data {
	int m_pipefd[2];
	pid_t m_pid;

	Process_Data() :m_pid(-1) {}
};


//进程池类
template<typename User_Data>
class Process_Pool {
public:
	static constexpr int MAX_PROCESS_NUM = 16;
	static constexpr int USER_PER_PROCESS = 65536;
	static constexpr int MAX_EVENT_NUM = 1024;

	~Process_Pool() { delete[] m_sub_proccess; }

	static Process_Pool<User_Data>* create(int listenfd, int procnum = 4);
	void run();

private:
	Process_Pool(int listenfd, int procnum);

	void setup_sigpipe();
	void run_parent();
	void run_child();

private:
	static Process_Pool<User_Data>* m_instance; //保证进程池单例
	Process_Data* m_sub_proccess;	//进程信息表
	int m_process_num;				//子进程数
	int m_process_idx;				//当前子进程标号
	int m_listenfd;
	int m_epfd;
};


template<typename User_Data>
Process_Pool<User_Data>* 
Process_Pool<User_Data>::m_instance = nullptr;


/* 构造进程池，池中的每一个进程与父进程使用管道进行相连 */
template<typename User_Data>
Process_Pool<User_Data>::Process_Pool(int listenfd, int procnum) :
	m_process_num(procnum), m_process_idx(-1), m_listenfd(listenfd), m_epfd(-1) {
	if (procnum <= 0 && procnum > MAX_PROCESS_NUM)
		err_quit("process number oversize");
	m_sub_proccess = new Process_Data[procnum];

	for (int i = 0; i < procnum; ++i) {
		if (socketpair(AF_UNIX, SOCK_STREAM, 0, m_sub_proccess[i].m_pipefd) == -1)
			err_sys("socketpair error");

		if ((m_sub_proccess[i].m_pid = fork()) == -1)
			err_sys("fork error");
		else if (m_sub_proccess[i].m_pid == 0) {
			close(m_sub_proccess[i].m_pipefd[0]);
			m_process_idx = i;
			break;
		}
		else close(m_sub_proccess[i].m_pipefd[1]);
	}
}


/* 以单例模式创建线程池 */
template<typename User_Data>
Process_Pool<User_Data>*
Process_Pool<User_Data>::create(int listenfd, int procnum) {
	if (!m_instance)
		m_instance = new Process_Pool<User_Data>(listenfd, procnum);
	return m_instance;
}


template<typename User_Data>
void Process_Pool<User_Data>::run() {
	if (m_process_idx == -1) run_parent();
	else run_child();
}


static void sig_handler(int signo) {
	int errno_save = errno;
	if (write(sig_pipefd[1], &signo, 1) != 1)
		err_sys("write error");
	errno = errno_save;
}


/* 创建epoll对象，并统一事件源 */
template<typename User_Data>
void Process_Pool<User_Data>::setup_sigpipe() {
	if ((m_epfd = epoll_create(5)) == -1)
		err_sys("epoll_create error");
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sig_pipefd) == -1)
		err_sys("socketpair error");
	setnonblocking(sig_pipefd[1]);
	add2epoll(m_epfd, sig_pipefd[0]);

	mysignal1(SIGINT, sig_handler);
	mysignal1(SIGTERM, sig_handler);
	mysignal1(SIGCHLD, sig_handler);
	mysignal1(SIGPIPE, SIG_IGN);
}


//父进程负责在新客户请求到来的时候选择一个子进程为之服务
template<typename User_Data>
void Process_Pool<User_Data>::run_parent() {
	struct epoll_event events[MAX_EVENT_NUM];
	constexpr int new_conn = 1;
	int sub_proc_cnt = 0;
	bool stop = false;
	int sockfd, nret;
	ssize_t nread;
	pid_t pid;

	setup_sigpipe();
	add2epoll(m_epfd, m_listenfd);

	while (!stop) {
		if ((nret = epoll_wait(m_epfd, events, MAX_EVENT_NUM, -1)) == -1) {
			if (errno == EINTR) continue;
			err_sys("epoll_wait error");
		}

		for (int i = 0; i < nret; ++i) {
			sockfd = events[i].data.fd;

			//以Round Robin算法选择一个子进程为客户服务
			if (sockfd == m_listenfd) {
#ifdef DEBUG
				std::cout << "send request to child " << sub_proc_cnt
					<< ", pid " << m_sub_proccess[sub_proc_cnt].m_pid << std::endl;
#endif // DEBUG
				if (write(m_sub_proccess[sub_proc_cnt].m_pipefd[0],
					&new_conn, sizeof(int)) != sizeof(int))
					err_sys("write error");
				sub_proc_cnt = (sub_proc_cnt + 1) % m_process_num;
			}
			//处理信号事件
			else if (sockfd == sig_pipefd[0] && (events[i].events & EPOLLIN)) {
				char signals[1024];
				if ((nread = read(sockfd, &signals, sizeof(signals))) <= 0)
					continue;
				for (int j = 0; j < nret; ++j) {
					switch (signals[j]) {
					case SIGCHLD:
						while ((pid = waitpid(-1, nullptr, WNOHANG)) > 0) {
							for (int k = 0; k < m_process_num; ++k) {
								if (m_sub_proccess[k].m_pid == pid) {
									std::cout << "child " << k << " joined" << std::endl;
									close(m_sub_proccess[k].m_pipefd[0]);
									m_sub_proccess[k].m_pid = -1;
								}
							}
						}
						stop = true;
						for (int k = 0; k < m_process_num; ++k) {
							pid = m_sub_proccess[k].m_pid;
							if (pid != -1)
								stop = false;
						}
						break;
					case SIGINT:case SIGTERM:
						std::cout << "\nserver closing..." << std::endl;
						for (int k = 0; k < m_process_num; ++k) {
							pid = m_sub_proccess[k].m_pid;
							if (pid != -1)
								kill(pid, SIGTERM);
						}
						break;
					default:
						break;
					}
				}
			}
		}
	}

	close(m_listenfd);
	close(m_epfd);
	exit(EXIT_SUCCESS);
}


//子进程负责接收新的客户，并对其进行服务
template<typename User_Data>
void Process_Pool<User_Data>::run_child() {
	struct epoll_event events[MAX_EVENT_NUM];
	int nret, sockfd, pipefd, connfd;
	struct sockaddr_in cliaddr;
	bool stop = false;
	socklen_t clilen;
	ssize_t nread;

	setup_sigpipe();
	pipefd = m_sub_proccess[m_process_idx].m_pipefd[1];
	add2epoll(m_epfd, pipefd);
	User_Data* users = new User_Data[USER_PER_PROCESS];

	while (!stop) {
		if ((nret = epoll_wait(m_epfd, events, MAX_EVENT_NUM, -1)) == -1) {
			if (errno == EINTR) continue;
			err_sys("epoll_wait error");
		}

		for (int i = 0; i < nret; ++i) {
			sockfd = events[i].data.fd;

			//接受新的客户连接
			if (pipefd == sockfd && (events[i].events & EPOLLIN)) {
				int client;
				nread = read(sockfd, &client, sizeof(client));
				if ((nread == -1 && errno != EINTR) || nread == 0)
					continue;

				clilen = sizeof(cliaddr);
				if ((connfd = accept(m_listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1)
					err_sys("accept error");
				std::cout << currtime("%T") << ": new connection from " <<
					sock_ntop((const struct sockaddr*)&cliaddr, clilen) << std::endl;

				add2epoll(m_epfd, connfd);
				//记录用户信息
				users[connfd].init(m_epfd, connfd, cliaddr);
			}
			//收到来自父进程发送来的信号
			else if (sockfd == sig_pipefd[0] && (events[i].events & EPOLLIN)) {
				char signals[1024];
				if ((nread = read(sockfd, signals, sizeof(signals))) <= 0)
					continue;
				for (int j = 0; j < nread; ++j) {
					switch (signals[j]) {
					case SIGCHLD:
						while (waitpid(-1, nullptr, WNOHANG) > 0);
						break;
					case SIGINT:case SIGTERM:
						stop = true;
						break;
					default:
						break;
					}
				}
			}
			//处理已连接客户的请求
			else if (events[i].events & EPOLLIN)
				users[sockfd].process();
		}
	}

	delete[] users;
	close(pipefd);
	close(m_listenfd);
	close(m_epfd);
	exit(EXIT_SUCCESS);
}


#endif // !PROCESSPOOL_H_

#include "MyUNP.h"

/** 
 * 聊天室服务程序，将从客户收到的消息发送
 * 给其他除发送者用户之外的其他用户
 */

#define USER_LIMITS 6
#define FD_LIMITS 32

/* 记录相应聊天室客户信息，可以认为是
	套接字描述符到客户信息的HashMap */
struct client_data {
	//struct sockaddr_in cliaddr;
	char* writebuf;
	char buf[SBUFSIZE];
} users[FD_LIMITS];



int main(int argc, char* argv[])
{
	int err, nret, connfd, listenfd, user_cnt = 0;
	struct pollfd fds[USER_LIMITS + 1];
	struct sockaddr_in cliaddr;
	socklen_t clilen, len;
	ssize_t nread;

	if (argc != 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	listenfd = tcp_listen(NULL, argv[1], NULL);
	fds[0].fd = listenfd;
	fds[0].events = POLLIN | POLLERR;
	fds[0].revents = 0;
	for (int i = 1; i <= USER_LIMITS; ++i) {
		fds[i].fd = -1;
		fds[i].events = 0;
	}

	for (;;) {
		if ((nret = poll(fds, user_cnt + 1, -1)) == -1)
			err_sys("poll error");

		for (int i = 0; i < user_cnt + 1; ++i) {
			//监听套接字可用
			if (fds[i].fd == listenfd && (fds[i].revents & POLLIN)) {
				clilen = sizeof(cliaddr);
				if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1)
					err_sys("accept error");

				//用户数量超限
				if (user_cnt >= USER_LIMITS) {
					if (write(connfd, "too many users!", 16) != 16)
						err_sys("write error");
					if (close(connfd) == -1)
						err_sys("close error");
					continue;
				}
				/* 更新fds结构数组和users数组 */
				user_cnt++;
				fds[user_cnt].fd = connfd;
				fds[user_cnt].events = POLLIN | POLLRDHUP | POLLERR;
				fds[user_cnt].revents = 0;
				//users[connfd].cliaddr = cliaddr;
				if (set_fl(connfd, O_NONBLOCK) == -1)
					err_sys("set_fl error");
				printf("comes a new user(%s), now has %d users\n",
					sock_ntop((const struct sockaddr*)&cliaddr, clilen),user_cnt);
			}
			//套接字上发生了错误
			else if (fds[i].revents & POLLERR) {
				len = sizeof(int);
				if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &err, &len) == -1)
					err_sys("getsockopt error");
				err_cont(err, "SO_ERROR");
			}
			//连接被对方关闭，此时也应该相应的对fds数组和users数组进行更新
			else if (fds[i].revents & POLLRDHUP) {
				users[fds[i].fd] = users[fds[user_cnt].fd];
				if (close(fds[i].fd) == -1)
					err_sys("close error");
				fds[i--] = fds[user_cnt--];
				printf("a client left\n");
			}
			else if (fds[i].revents & POLLIN) {
				connfd = fds[i].fd;

				if ((nread = read(connfd, users[connfd].buf, sizeof(users[connfd].buf) - 1)) == -1) {
					if (errno != EWOULDBLOCK)
						err_sys("read error");
				}
				else if(nread==0){}
				users[connfd].buf[nread] = 0;
				//对其他套接字注册写事件，暂时取消读事件
				for (int j = 1; j <= user_cnt; ++j) {
					if (fds[i].fd != fds[j].fd) {
						fds[j].events &= ~POLLIN;
						fds[j].events |= POLLOUT;
						users[fds[j].fd].writebuf = users[connfd].buf;
					}
				}
			}
			else if (fds[i].revents & POLLOUT) {
				connfd = fds[i].fd;
				if (!users[connfd].writebuf)
					continue;
				if (write(connfd, users[connfd].writebuf, strlen(users[connfd].writebuf)) 
					!= strlen(users[connfd].writebuf))
					err_sys("write error");
				users[connfd].writebuf = NULL;
				//恢复该套接字上的读事件，取消写事件
				fds[i].events |= POLLIN;
				fds[i].events &= ~POLLOUT;
			}
		}
	}
}
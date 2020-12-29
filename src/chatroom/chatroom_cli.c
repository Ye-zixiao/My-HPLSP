#include "MyUNP.h"

/**
 * 聊天室客户程序：标准输入数据发送给服务器
 */


int main(int argc, char* argv[])
{
	int pipefds[2], sockfd, nret;
	struct pollfd fds[2];
	char buf[BUFSIZE];
	ssize_t nread;

	if (argc < 3)
		err_quit("usage: %s <host/ip> <serv/port>", 
			basename(argv[0]));

	sockfd = tcp_connect(argv[1], argv[2]);
	if (pipe(pipefds) == -1)
		err_sys("pipe error");
	fds[0].fd = STDIN_FILENO;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = sockfd;
	fds[1].events = POLLIN | POLLRDHUP;
	fds[1].revents = 0;

	for (;;) {
		printf("%40sinput: "," ");
		fflush(stdout);
		if ((nret = poll(fds, 2, -1)) == -1)
			err_sys("poll error");

		if (fds[1].revents & POLLRDHUP) {
			printf("server close the connection\n");
			break;
		}
		else if (fds[1].revents & POLLIN) {
			if ((nread = read(sockfd, buf, sizeof(buf) - 1)) == -1)
				err_sys("read error");
			buf[nread] = 0;
			printf("\n%s\nother: %s", currtime("%T"), buf);
		}
		if (fds[0].revents & POLLIN) {
			if ((nread = splice(STDIN_FILENO, NULL, pipefds[1], NULL,
				32768, SPLICE_F_MORE | SPLICE_F_MOVE)) == -1)
				err_sys("splice error");
			if (splice(pipefds[0], NULL, sockfd, NULL, nread,
				SPLICE_F_MORE | SPLICE_F_MOVE) == -1)
				err_sys("splice error");
		}
	}
}
#include "MyUNP.h"

#define BUFFER_SIZE 1024

int main(int argc, char* argv[])
{
	struct sockaddr_in svaddr;
	int listenfd, connfd;
	const int on = 1;

	if (argc < 2)
		err_quit("usage: %s <port>", basename(argv[0]));

	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err_sys("socket error");
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1)
		err_sys("setsockopt error");
	bzero(&svaddr, sizeof(svaddr));
	svaddr.sin_family = AF_INET;
	svaddr.sin_port = htons(atoi(argv[1]));
	svaddr.sin_addr.s_addr = INADDR_ANY;
	if (bind(listenfd, (struct sockaddr*)&svaddr, sizeof(svaddr)) == -1)
		err_sys("bind error");
	if (listen(listenfd, 5) == -1)
		err_sys("listen error");

	if ((connfd = accept(listenfd, NULL, NULL)) == -1)
		err_sys("accept error");
	if (dup2(connfd, STDOUT_FILENO) == -1)
		err_sys("dup2 error");
	if (close(connfd) == -1)
		err_sys("close error");
	printf("abcd\n");

	exit(EXIT_SUCCESS);
}
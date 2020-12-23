#include "MyUNP.h"

#define BUFFER_SIZE 1024

int main(int argc, char* argv[])
{
	struct sockaddr_in svaddr;
	char buf[BUFFER_SIZE];
	int listenfd, connfd;
	int recv_buf_size;
	socklen_t len;

	if (argc < 3)
		err_quit("usage: %s <port> <recv_buf_size>", basename(argv[0]));

	recv_buf_size = atoi(argv[2]);
	len = sizeof(int);
	if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err_sys("socket error");
	if (setsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, len) == -1)
		err_sys("setsockopt error");
	if (getsockopt(listenfd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size, &len) == -1)
		err_sys("getsockopt error");
	printf("received buffer size: %d\n", recv_buf_size);

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
	while (read(connfd, buf, BUFFER_SIZE) > 0);
	exit(EXIT_SUCCESS);
}
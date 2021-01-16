#include "MyUNP.h"
 
#define BUFFER_SIZE 4096

int main(int argc, char* argv[])
{
	struct sockaddr_in svaddr;
	int sockfd, snd_buf_sz;
	char buf[BUFFER_SIZE];
	socklen_t len;

	if (argc < 4)
		err_quit("usage: %s <ip> <port> <snd_buf_size>", basename(argv[0]));

	snd_buf_sz = atoi(argv[3]);
	len = sizeof(int);
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err_sys("socket error");
	if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &snd_buf_sz, len) == -1)
		err_sys("setsockopt error");
	if (getsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &snd_buf_sz, &len) == -1)
		err_sys("getsockopt error");
	printf("send buffer size: %d\n", snd_buf_sz);

	bzero(&svaddr, sizeof(svaddr));
	svaddr.sin_family = AF_INET;
	svaddr.sin_port = htons(atoi(argv[2]));
	if (inet_pton(AF_INET, argv[1], &svaddr.sin_addr) <= 0)
		err_sys("inet_pton error");
	if (connect(sockfd, (struct sockaddr*)&svaddr, sizeof(svaddr)) == -1)
		err_sys("connect error");

	memset(buf, 'c', BUFFER_SIZE);
	if (write(sockfd, buf, BUFFER_SIZE) != BUFFER_SIZE)
		err_sys("write error");
	exit(EXIT_SUCCESS);
}

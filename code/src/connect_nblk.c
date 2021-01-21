#include "MyUNP.h"


int main(int argc, char* argv[])
{
	struct sockaddr_in svaddr;
	int sockfd;

	if (argc != 3)
		err_quit("usage: %s <ip> <port>", basename(argv[0]));

	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		err_sys("socket error");
	bzero(&svaddr, sizeof(svaddr));
	svaddr.sin_family = AF_INET;
	svaddr.sin_port = htons((in_port_t)atoi(argv[2]));
	if (inet_pton(AF_INET, argv[1], &svaddr.sin_addr) <= 0)
		err_sys("inet_pton error");

	if (connect_nblk(sockfd, (const struct sockaddr*)&svaddr, sizeof(svaddr), 5) == -1)
		err_sys("connect_nblk error");
	if (close(sockfd) == -1)
		err_sys("close error");
	exit(EXIT_SUCCESS);
}
#include "MyUNP.h"

/**
 * 根据主机+服务获取服务器的协议地址，并以此创建一个已连接UDP
 * 套接字，然后返回给客户进程
 **/
int udp_connect(const char* host, const char* serv) {
	struct addrinfo hints, * res, * ressave;
	int sockfd, gerr;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	if ((gerr = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("udp_connect error for %s, %s: %s",
			host, serv, gai_strerror(gerr));

	ressave = res;
	do {
		if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
			continue;
		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;
		if (close(sockfd) == -1)
			err_sys("close error");
	} while ((res = res->ai_next) != NULL);
	if (res == NULL)
		err_sys("udp_connect error for %s, %s", host, serv);

	freeaddrinfo(ressave);
	return sockfd;
}
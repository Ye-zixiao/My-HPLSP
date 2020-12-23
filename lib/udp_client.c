#include "MyUNP.h"


/**
 * 根据指定主机+服务查询服务器协议地址，并创建一个未连接UDP套接字
 */
int udp_client(const char* host, const char* serv,
		struct sockaddr** saptr, socklen_t* lenp) {
	struct addrinfo hints, * res, * ressave;
	int sockfd, gerr;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	if ((gerr = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("udp_client error for %s, %s: %s",
			host, serv, gai_strerror(gerr));

	ressave = res;
	do {
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (socket >= 0)break;
	} while ((res = res->ai_next) != NULL);
	if (res == NULL)
		err_sys("udp_client error for %s, %s", host, serv);

	if ((*saptr = malloc(res->ai_addrlen)) == NULL)
		err_sys("malloc error");
	memcpy(*saptr, res->ai_addr, res->ai_addrlen);
	*lenp = res->ai_addrlen;

	freeaddrinfo(ressave);
	return sockfd;
}
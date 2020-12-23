#include "MyUNP.h"


/**
 * 根据主机+服务获取服务器应该绑定的协议地址，然后创建UDP套接字
 * 并与上述地址进行绑定，成功后返回该UDP套接字描述符
 **/
int udp_server(const char* host, const char* serv, socklen_t* lenp) {
	struct addrinfo hints, * res, * ressave;
	int sockfd, gerr;

	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	if ((gerr = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("udp_server error for %s, %s: %s",
			host, serv, gai_strerror(gerr));

	ressave = res;
	do {
		if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
			continue;
		if (bind(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;
		if (close(sockfd) == -1)
			err_sys("close error");
	} while ((res = res->ai_next) != NULL);
	if (res == NULL)
		err_sys("udp_server error for %s, %s", host, serv);

	if (lenp != NULL)
		*lenp = res->ai_addrlen;
	freeaddrinfo(ressave);
	return sockfd;
}
#include "MyUNP.h"


/**
 * 仅供TCP客户进程使用.
 * 先根据主机+服务获取协议地址，然后直接为客户进程创建一个连接套接字，
 * 并在成功时返回这个连接套接字描述符
 **/
int tcp_connect(const char* host, const char* serv) {
	struct addrinfo hints, * res, * ressave;
	int sockfd, gerr;

	bzero(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((gerr = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("tcp_connect error for %s, %s: %s",//其实我是想返回-1的
			host, serv, gai_strerror(gerr));

	ressave = res;
	do {
		if ((sockfd = socket(res->ai_family, res->ai_socktype, 
				res->ai_protocol))== -1)
			continue;
		if (connect(sockfd, res->ai_addr, res->ai_addrlen) == 0)
			break;
		if (close(sockfd) == -1)
			err_sys("close error");
	} while ((res = res->ai_next) != NULL);
	if (res == NULL)
		err_sys("tcp_connect error for %s, %s", host, serv);

	freeaddrinfo(ressave);
	return sockfd;
}

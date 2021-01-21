#include "MyUNP.h"


/**
 * 仅供服务进程使用.
 * 根据指定的主机+服务获取协议地址，并完成套接字创建、地址
 * 绑定、转成监听的操作，若成功返回套接字描述符
 **/
int tcp_listen(const char* host, const char* serv, socklen_t* addrlen) {
	struct addrinfo hints, * res, * ressave;
	int listenfd, gerr;
	const int on = 1;

	bzero(&hints, sizeof(hints));
	hints.ai_flags = AI_PASSIVE;
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((gerr = getaddrinfo(host, serv, &hints, &res)) != 0)
		err_quit("tcp_listen erorr for %s, %s: %s",
			host == NULL ? "NULL" : host, serv, gai_strerror(gerr));

	ressave = res;
	do {
		if ((listenfd = socket(res->ai_family,
				res->ai_socktype, res->ai_protocol)) == -1)
			continue;
		if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int)) == -1)
			err_sys("setsockopt error");
		if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
			break;
		if (close(listenfd) == -1)
			err_sys("close error");
	} while ((res = res->ai_next) != NULL);
	if (res == NULL)
		err_sys("tcp_listen error for %s, %s", host, serv);

	if (listen(listenfd, LISTENQ) == -1)
		err_sys("listen error");
	if (addrlen != NULL)
		*addrlen = res->ai_addrlen;
	freeaddrinfo(ressave);
	return listenfd;
}
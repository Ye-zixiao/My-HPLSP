#include "MyUNP.h"


/**
 * 供任何用户获取协议地址使用.
 * 使用主机+服务返回addrinfo结构链表指针，函数仅限指定地址族
 * 和套接字类型，若失败返回NULL
 **/
struct addrinfo*
host_serv(const char* host, const char* serv, int family, int socktype) {
	struct addrinfo hint, * res;

	bzero(&hint, sizeof(hint));
	hint.ai_flags = AI_CANONNAME;
	hint.ai_family = family;
	hint.ai_socktype = socktype;
	if (getaddrinfo(host, serv, &hint, &res) != 0)
		return NULL;
	return res;
}
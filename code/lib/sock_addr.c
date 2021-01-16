#include "MyUNP.h"


/**
 * 将通用地址结构(网络字节序)转换为表达(presentation)格式，
 * 它既能够处理IPv4地址结构也能够处理IPv6地址结构
 */
char* sock_ntop(const struct sockaddr* sockaddr, socklen_t addrlen) {
	static char buf[128];
	char port[8];

	if (sockaddr->sa_family == AF_INET) {
		const struct sockaddr_in* sin = (const struct sockaddr_in*)sockaddr;

		if (inet_ntop(AF_INET, &sin->sin_addr, buf, sizeof(buf)) == NULL)
			return NULL;
		if (ntohs(sin->sin_port) != 0) {
			snprintf(port, sizeof(port), ":%d", ntohs(sin->sin_port));
			strcat(buf, port);
		}
		return buf;
	}
	else if (sockaddr->sa_family == AF_INET6) {
		const struct sockaddr_in6* sin6 = (const struct sockaddr_in6*)sockaddr;

		buf[0] = '[';
		if (inet_ntop(AF_INET6, &sin6->sin6_addr, buf + 1, sizeof(buf) - 1) == NULL)
			return NULL;
		if (ntohs(sin6->sin6_port) != 0) {
			snprintf(port, sizeof(port), "]:%d", htons(sin6->sin6_port));
			strcat(buf, port);
			return buf;
		}
		return buf + 1;
	}
	//其他协议暂时不实现
	errno = EAFNOSUPPORT;
	return NULL;
}
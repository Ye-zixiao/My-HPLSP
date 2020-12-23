#include "MyUNP.h"


/**
 * 支持字符串形式的服务名称或者端口号获取服务名->端口号映射关系
 */
struct servent* getserv(const char* name_or_port, const char* protoname) {
	struct servent* sptr;
	in_port_t port;

	if ((sptr = getservbyname(name_or_port, protoname)) == NULL) {
		port = atoi(name_or_port);
		if ((sptr = getservbyport(htons(port), protoname)) == NULL)
			return NULL;
	}
	return sptr;
}
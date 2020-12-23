#include "MyUNP.h"


/* 传入一个struct in_addr的数组，并根据指定的字符串提取出一个
	或者多个IPv4地址存放在上述数组中 */
int gethost(struct in_addr* addrArr, int n, const char* host) {
	struct hostent* hptr;
	char** pptr;
	int i;

	if (inet_aton(host, &addrArr[0]) == 1)
		i = 1;
	else {
		if ((hptr = gethostbyname(host)) == NULL)
			return -1;

		pptr = hptr->h_addr_list;
		for (i = 0; pptr[i] != NULL; ++i) {
			if (i >= n) return -2;
			memcpy(&addrArr[i], *pptr, sizeof(struct in_addr));
		}
	}
	return i;
}


/* gethost的另一个版本，这个函数使用方法与gethostbyname()一致，且与getserv()
	函数形式对齐。既可以处理真正的主机名，也可以处理点分十进制数串  */
struct hostent* gethost1(const char* host) {
	static struct in_addr *addrpArr[2], addrbuf;
	static struct hostent hostent;
	struct hostent* hptr;

	if (inet_pton(AF_INET, host, &addrbuf) > 0) {
		addrpArr[0] = &addrbuf;
		addrpArr[1] = NULL;

		hostent.h_name = "";
		hostent.h_aliases = NULL;
		hostent.h_addrtype = AF_INET;
		hostent.h_length = 4;
		hostent.h_addr_list = (char**)addrpArr;
		hptr = &hostent;
	}
	else {
		if ((hptr = gethostbyname(host)) == NULL)
			return NULL;
	}
	return hptr;
}
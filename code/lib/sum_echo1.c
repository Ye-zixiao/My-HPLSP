#include "MyUNP.h"


/**
 * 从客户进程中读取两个数值字符串，转换成数值后
 * 将相加的结果返回给客户进程
 */
void sum_echo1(int sockfd) {
	char line[MAXLINE];
	long arg1, arg2;
	size_t nread, len;

	for (;;) {
		if ((nread = readline(sockfd, line, MAXLINE)) == 0)
			return;
		else if (nread < 0)
			err_sys("readline error");

		if (sscanf(line, "%ld%ld", &arg1, &arg2) == 2)
			snprintf(line, sizeof(line), "%ld\n", arg1 + arg2);
		else
			snprintf(line, sizeof(line), "input error\n");
		len = strlen(line);
		if (write(sockfd, line, len) != len)
			err_sys("write error");
	}
}

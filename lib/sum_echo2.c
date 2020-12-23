#include "MyUNP.h"

struct args {
	long arg1;
	long arg2;
};

struct result {
	long sum;
};


/**
 * 客户进程读取用户输入的两个数值字符串，然后以二进制形式发送
 * 给服务进程
 */
void sum_cli2(int sockfd, FILE* fp) {
	char sendline[MAXLINE];
	struct result result;
	struct args args;
	ssize_t nread;

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		if (sscanf(sendline, "%ld%ld", &args.arg1, &args.arg2) != 2) {
			printf("invalid input: %s", sendline);
			continue;
		}
		if (write(sockfd, &args, sizeof(args)) != sizeof(args))
			err_sys("write error");
		if ((nread = readn(sockfd, &result, sizeof(result))) < 0)
			err_sys("readline error");
		else if (nread == 0)
			err_quit("sum_cli2: server prematurely terminated");
		printf("%ld\n", result.sum);
	}
	if (ferror(fp))
		err_sys("fgets error");
}


/**
 * 服务进程从客户进程中读入二进制形式（结构）的两个数值，然后
 * 将其相加然后返回给客户进程（需要注意的是这种形式的服务在可
 * 移植性上很差，因为可能在收到字节序的影响造成不同数据误解释）
 */
void sum_echo2(int sockfd) {
	struct args args;
	struct result result;
	ssize_t nread;

	for (;;) {
		if ((nread = readn(sockfd, &args, sizeof(args))) < 0)
			err_sys("readline error");
		else if (nread == 0)
			return;
		result.sum = args.arg1 + args.arg2;
		if (write(sockfd, &result, sizeof(result)) != sizeof(result))
			err_sys("write error");
	}
}
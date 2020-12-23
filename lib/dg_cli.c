#include "MyUNP.h"


static void sig_alrm(int signo) {
	return;
}


/**
 * UNP书中提供的纯真版本 */
void dg_cli(int sockfd, FILE* fp,
		const struct sockaddr* svaddr, socklen_t svlen) {
	char sendline[MAXLINE], recvline[MAXLINE + 1];
	ssize_t nrecv;

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		if (sendto(sockfd, sendline, strlen(sendline), 0, svaddr, svlen) == -1)
			err_sys("sendto error");
		if ((nrecv = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) == -1)
			err_sys("recvfrom error");
		recvline[nrecv] = 0;
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
	if (ferror(fp))
		err_sys("fgets error");
}



/**
 * 向服务进程发送UDP数据报并试图获取回射数据 */
void dg_clit0(int sockfd, FILE* fp, 
		const struct sockaddr* svaddr, socklen_t svlen) {
	char sendline[MAXLINE], recvline[MAXLINE + 1];
	Sigfunc* old_sigfunc;
	ssize_t nrecv;

	if ((old_sigfunc = mysignal(SIGALRM, sig_alrm)) == SIG_ERR)
		err_sys("mysignal error");

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		if (sendto(sockfd, sendline, strlen(sendline), 0, svaddr, svlen) == -1)
			err_sys("sendto error");

		/* 若5秒之内不能接收到返回消息则执行由SIGALRM中断返回，
			并打印提示信息 */
		alarm(5);
		if ((nrecv = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) == -1) {
			if (errno == EINTR)
				err_msg("socket timeout");
			else err_sys("recvfrom error");
		}
		else {
			alarm(0);
			recvline[nrecv] = 0;
			if (fputs(recvline, stdout) == EOF)
				err_sys("fputs error");
		}
	}
	if (ferror(fp))
		err_sys("fgets error");
}



/**
 * dg_cli函数的再版，不过将其中的等待套接字描述符可读超时机制由
 * 原来的信号中断完成改成了由select封装的readable_timeo()函数完成
 */
void dg_clit1(int sockfd, FILE* fp,
		const struct sockaddr* svaddr, socklen_t svlen) {
	char sendline[MAXLINE], recvline[MAXLINE + 1];
	ssize_t nrecv;

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		if (sendto(sockfd, sendline, strlen(sendline), 0, svaddr, svlen) == -1)
			err_sys("sendto error");

		/* 定时5秒，若超过了这个时间，readable_timeo内部封装的
			select就会返回0，表示5秒内指定的套接字描述符并没有
			准备好，此时后面的recvfrom当然不能调用 */
		if (readable_timeo(sockfd, 5) <= 0)
			err_msg("socket timeout");
		else {
			if ((nrecv = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) == -1)
				err_sys("recvfrom error");
			recvline[nrecv] = '\0';
			if (fputs(recvline, stdout) == EOF)
				err_sys("fputs error");
		}
	}
}



/**
 * dg_cli函数的再版，不过将其中的等待套接字描述符可读超时机制由
 * 原来的信号中断完成改成了由套接字选项SO_RCVTIMEO来完成
 */
void dg_clit2(int sockfd, FILE* fp,
		const struct sockaddr* svaddr, socklen_t svlen) {
	char sendline[MAXLINE], recvline[MAXLINE + 1];
	struct timeval timebuf;
	ssize_t nrecv;

	timebuf.tv_sec = 5;
	timebuf.tv_usec = 0;
	if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timebuf, sizeof(timebuf)) == -1)
		err_sys("setsockopt error");

	while (fgets(sendline, MAXLINE, fp) != NULL) {
		if (sendto(sockfd, sendline, strlen(sendline), 0, svaddr, svlen) == -1)
			err_sys("sendto error");
		
		if ((nrecv = recvfrom(sockfd, recvline, MAXLINE, 0, NULL, NULL)) == -1) {
			if (errno == EWOULDBLOCK) {
				err_msg("socket timeout");
				continue;
			}
			err_sys("recvfrom error");
		}
		recvline[nrecv] = '\0';
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
}



/* 具有验证功能的dg_cli()函数，只接收来自服务器协议地址的数据报 */
void dg_cli1(int sockfd, FILE* fp,
		const struct sockaddr* svaddr, socklen_t svlen) {
	char sendline[MAXLINE], recvline[MAXLINE + 1];
	struct sockaddr* replyaddr;
	ssize_t nrecv;
	socklen_t len;

	if ((replyaddr = malloc(svlen)) == NULL)
		err_sys("malloc error");
	while (fgets(sendline, MAXLINE, fp) != NULL) {
		len = svlen;
		if (sendto(sockfd, sendline, strlen(sendline), 0, svaddr, len) == -1)
			err_sys("sendto error");
		if ((nrecv = recvfrom(sockfd, recvline, MAXLINE, 0, replyaddr, &len)) == -1)
			err_sys("recvfrom error");
		if (len != svlen || memcmp(replyaddr, svaddr, len) != 0) {
			printf("reply from %s (ignored)\n", sock_ntop(replyaddr, len));
			continue;
		}
		recvline[nrecv] = '\0';
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
}



/* 使用已连接UDP套接字执行发送并接收回射数据的功能 */
void dg_cli2(int sockfd, FILE* fp, 
		const struct sockaddr* svaddr, socklen_t svlen) {
	char sendline[MAXLINE], recvline[MAXLINE + 1];
	ssize_t nrecv;

	if (connect(sockfd, svaddr, svlen) == -1)
		err_sys("connect error");
	while (fgets(sendline, MAXLINE, fp) != NULL) {
		if (write(sockfd, sendline, strlen(sendline)) == -1)
			err_sys("read error");
		if ((nrecv = read(sockfd, recvline, MAXLINE)) == -1)
			err_sys("read error");
		recvline[nrecv] = 0;
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	}
}



#define NDG 2000
#define DGLEN 1400

/**
 * 对目标UDP套接字发送大量的UDP数据报，试图淹没接收端
 */
void dg_clix(int sockfd, FILE* fp,
	const struct sockaddr* svaddr, socklen_t svlen) {
	char sendline[DGLEN];

	for (int i = 0; i < NDG; ++i)
		if (sendto(sockfd, sendline, DGLEN, 0, svaddr, svlen) == -1)
			err_sys("sendto error");
}
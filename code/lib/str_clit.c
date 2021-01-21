#include "MyUNP.h"


static int iseof;
static int thread_fd;
static FILE* thread_fp;


static void* thread_func(void* arg) {
	char sendline[MAXLINE];

	while (fgets(sendline, MAXLINE, thread_fp) != NULL) {
		if (write(thread_fd, sendline, strlen(sendline)) != strlen(sendline))
			err_sys("write error");
	}
	if (ferror(thread_fp))
		err_sys("fgets error");
	if (shutdown(thread_fd, SHUT_WR) == -1)
		err_sys("shutdown error");
	iseof = 1;
	return (void*)NULL;
}


/**
 * 使用线程技术实现的str_cli函数，其中主线程负责从套接字中读取
 * 返回的数据并打印，而另一个新建线程负责将数据从文件指针指向的
 * 流中读取并发送给对端
 */
void str_clit(int sockfd, FILE* fp) {
	char recvline[MAXLINE];
	pthread_t tid;
	ssize_t nread;
	int err;

	thread_fd = sockfd;
	thread_fp = fp;
	if ((err = pthread_create(&tid, NULL, thread_func, (void*)NULL)) != 0)
		err_exit(err, "pthread_create error");
	while ((nread = readline(sockfd, recvline, MAXLINE)) > 0)
		if (fputs(recvline, stdout) == EOF)
			err_sys("fputs error");
	if (nread == 0 && iseof == 0) {
		//可能err_quit()函数过长的执行流程会给另一个线程上下文切换的机会
		//err_quit("server terminated prematurely");
		fprintf(stderr, "server terminated prematurely\n");
		exit(EXIT_FAILURE);
	}
	if (nread < 0)
		err_sys("readline error");
}
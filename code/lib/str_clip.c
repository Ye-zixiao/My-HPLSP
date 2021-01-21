#include "MyUNP.h"


static void sig_usr(int signo) {
	return;
}

/**
 * 使用两个进程完成数据的读/写的str_cli版本，其中父进程负责向服务进程
 * 发送读自stdin的数据，而子进程负责从服务进程中读取返回的数据并输出到
 * stdout。这种方式我不推荐使用
 */
void str_clip(int sockfd, FILE* fp) {
	char sendline[MAXLINE], recvline[MAXLINE];
	pid_t pid;

	if (mysignal(SIGUSR1, sig_usr) == SIG_ERR)
		err_sys("mysignal error");

	if ((pid = fork()) < 0)
		err_sys("fork error");
	else if (pid == 0) {
		while (readline(sockfd, recvline, MAXLINE) > 0)
			if (fputs(recvline, stdout) == EOF)
				err_sys("recvline error");
		kill(getppid(), SIGUSR1);
		exit(EXIT_SUCCESS);
	}
	
	while (fgets(sendline, MAXLINE, fp) != NULL)
		if (write(sockfd, sendline, strlen(sendline)) != strlen(sendline))
			err_sys("write error");
	if (shutdown(sockfd, SHUT_WR) == -1)
		err_sys("shutdown error");
	pause();
	return;
}
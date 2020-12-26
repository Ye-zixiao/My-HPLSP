#include "MyUNP.h"
#include <syslog.h>

#define MAXFD 64


int daemon_init(const char* pname, int facility) {
	pid_t pid;

	/* 创建子进程，终止父进程，保证子进程不为进程组的组长进程。
		这样我们就可以顺利的调用setsid()创建新的会话 */
	if ((pid = fork()) < 0)
		return -1;
	else if (pid)
		_exit(0);

	/* 创建新的会话，成为新会话的会话首进程。对SIGHUP信号做忽略
		处理，保证会话首进程(即父进程)终止后不会因systemd(init)
		进程的SIGHUP而终止 */
	umask(0);
	if (setsid() < 0)
		return -1;
	if (mysignal(SIGHUP, SIG_IGN) == SIG_ERR)
		return -1;

	/* 再创建子进程，终止父进程，保证子进程不为会话的会话首进程 */
	if ((pid = fork()) < 0)
		return -1;
	else if (pid)
		_exit(0);

	/* 关闭所有打开的描述符，并将stdin、stdout、stderr指向/dev/null，
		然后打开指定的*/
	daemon_proc = 1;
	chdir("/");
	for (int i = 0; i < MAXFD; ++i)
		close(i);
	open("/dev/null", O_RDONLY);
	open("/dev/null", O_RDWR);
	open("/dev/null", O_RDWR);
	openlog(pname, LOG_PID/* | LOG_CONS*/, facility);
	return 0;
}
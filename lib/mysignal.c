#include "MyUNP.h"


__sighandler_t mysignal(int signo, __sighandler_t func) {
	struct sigaction	 act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;
#endif
	}
	else {
#ifdef SA_RESTART
		/* 实际上Linux系统自带的signal函数会
			默认重启动被中断的系统调用 */
		act.sa_flags |= SA_RESTART;
#endif
	}
	
	if (sigaction(signo, &act, &oact) == -1)
		return SIG_ERR;
	return oact.sa_handler;
}
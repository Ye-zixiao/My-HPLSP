#include "MyUNP.h"
#include <stdarg.h>
#include <syslog.h>

int daemon_proc;

#define ERRMSG_LEN	128


/**
 * 处理真正的错误消息输出的功能
 * @param iserror   是否输出错误码提示消息
 * @param error		错误码
 * @param level     syslog消息级别
 * @param fmt       格式化字符串
 * @param ap        可变参数列表
 */
static void 
err_doit(int iserror, int error, int level, const char* fmt, va_list ap) {
	char buf[ERRMSG_LEN + 1];
	int slen;

	vsnprintf(buf, ERRMSG_LEN, fmt, ap);
	slen = strlen(buf);
	if (iserror)
		snprintf(buf + slen, ERRMSG_LEN - slen, ": %s", strerror(error));
	strcat(buf, "\n");

	//根据情况输出到syslog设施或者stderr中
	if (daemon_proc)
		syslog(level, buf);
	else {
		fflush(stdout);
		fputs(buf, stderr);
		fflush(NULL);
	}
}


/* 与系统调用相关的非致命错误，打印errno */
void err_ret(const char* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, errno, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}


/* 与系统调用相关的非致命错误，打印errno，不过该错误码
	errno是出错函数返回得到的，需要用户手动传入 */
void err_cont(int error, const char* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, error, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}


/* 与系统调用相关的致命错误，打印errno */
void err_sys(const char* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, errno, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}


/* 与系统调用相关的致命错误，打印errno，不过该错误码
	errno是出错函数返回得到，需要用户手动传入 */
void err_exit(int error, const char* fmt, ...) {
	va_list ap;
	
	va_start(ap, fmt);
	err_doit(1, error, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}


/* 与系统调用相关的致命错误，产生dump core */
void err_dump(const char* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	err_doit(1, errno, LOG_ERR, fmt, ap);
	va_end(ap);
	abort();
	exit(EXIT_FAILURE);
}




/* 与系统调用无关的非致命错误，不打印errno */
void err_msg(const char* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, 0, LOG_INFO, fmt, ap);
	va_end(ap);
	return;
}


/* 与系统调用无关的致命错误，不打印errno */
void err_quit(const char* fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
	err_doit(0, 0, LOG_ERR, fmt, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}


/* 自定义调试函数 */
void debug(void) {
	static int cnt = 0;
	fprintf(stderr, "get here(%d)?\n", cnt++);
}
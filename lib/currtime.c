#include "MyUNP.h"

#define TIMEBUFLEN 64


/**
 * 以指定字符串格式获取当前时间，非线程安全 
 */
const char* currtime(const char* fmt) {
	static char timebuf[TIMEBUFLEN];
	struct tm* ptm;
	time_t ct;

	time(&ct);
	if ((ptm = localtime(&ct)) == NULL)
		return NULL;
	if (strftime(timebuf, TIMEBUFLEN,
			fmt == NULL ? "%c" : fmt, ptm) == 0)
		return NULL;
	return timebuf;
}


/**
 * 以指定字符串格式获取当前时间，线程安全
 */
char* currtime_r(char* buf, size_t maxlen, const char* fmt) {
	struct tm* ptm;
	time_t ct;

	time(&ct);
	if ((ptm = localtime(&ct)) == NULL)
		return NULL;
	if (strftime(buf, maxlen, fmt == NULL ? "%c" : fmt, ptm) == 0)
		return NULL;
	return buf;
}


/**
 * 为了支持第16章较为精确的当前时间显示
 */
const char* currtime_p(const char* ignore) {
	static char timebuf[TIMEBUFLEN];
	struct timeval ct;
	struct tm* ptm;

	if (gettimeofday(&ct, NULL) == -1)
		return NULL;
	if ((ptm = localtime((const time_t*)&ct)) == NULL)
		return NULL;
	if (strftime(timebuf, TIMEBUFLEN, "%T", ptm) == 0)
		return NULL;
	snprintf(timebuf + 8, TIMEBUFLEN, ".%06ld", ct.tv_usec);
	return timebuf;
}
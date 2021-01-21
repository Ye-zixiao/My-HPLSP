#include "MyUNP.h"
#include <sys/resource.h>


void pr_cpu_time(void) {
	struct rusage myusage, childusage, threadusage;
	double user, sys;

	if (getrusage(RUSAGE_SELF, &myusage) == -1)
		err_sys("getrusage error");
	if (getrusage(RUSAGE_CHILDREN, &childusage) == -1)
		err_sys("getrusage error");
	if (getrusage(RUSAGE_THREAD, &threadusage) == -1)
		err_sys("getrusage error");

	user = (double)myusage.ru_utime.tv_sec +
		myusage.ru_utime.tv_usec / 1000000.0;
	user += (double)childusage.ru_utime.tv_sec +
		childusage.ru_utime.tv_usec / 1000000.0;
	user += (double)threadusage.ru_utime.tv_sec +
		threadusage.ru_utime.tv_usec / 1000000.0;
	sys = (double)myusage.ru_stime.tv_sec +
		myusage.ru_stime.tv_usec / 1000000.0;
	sys += (double)childusage.ru_stime.tv_sec +
		childusage.ru_stime.tv_usec / 1000000.0;
	sys += (double)threadusage.ru_stime.tv_sec +
		threadusage.ru_stime.tv_usec / 1000000.0;
	printf("\nuser time: %g, sys time: %g\n", user, sys);
}
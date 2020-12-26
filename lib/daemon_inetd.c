#include "MyUNP.h"
#include <syslog.h>


void daemon_inetd(const char* pname, int facility) {
	daemon_proc = 1;
	openlog(pname, LOG_PID, facility);
}
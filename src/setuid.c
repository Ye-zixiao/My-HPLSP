#include "MyUNP.h"

/**
 * 想试验setuid的功能，需要将该源文件生成的可执行文件的所有者和
 * 用户组设置为root，并加上SUID权限，然后以普通用户身份执行
 */

int main(void) {
	printf("ruid: %d, euid: %d\n", getuid(), geteuid());
	if (setuid(geteuid()) == -1)
		err_sys("setuid error");
	printf("ruid: %d, euid: %d\n", getuid(), geteuid());
	exit(EXIT_SUCCESS);
}

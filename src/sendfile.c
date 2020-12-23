#include "MyUNP.h"
#include <sys/sendfile.h>
#include <sys/stat.h>


#define DEFAULT_FMODE S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH

int main(int argc, char* argv[])
{
	struct stat statbuf;
	int infd, outfd;

	if (argc != 3)
		err_quit("usage: %s <infile> <outfile>", basename(argv[0]));

	if ((infd = open(argv[1], O_RDONLY)) == -1)
		err_sys("open error");
	if ((outfd = open(argv[2], O_CREAT | O_TRUNC | O_WRONLY, DEFAULT_FMODE)) == -1)
		err_sys("open error");
	if (fstat(infd, &statbuf) == -1)
		err_sys("fstat error");
	/* 这里的outfd若是一个套接字描述符，这就变成了向客户端发送文件数据
		一种最简便的方法了 */
	if (sendfile(outfd, infd, NULL, statbuf.st_size) == -1)
		err_sys("sendfile error");
	
	exit(EXIT_SUCCESS);
}
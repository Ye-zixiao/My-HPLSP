#include "MyUNP.h"

/**
 * 实现类似于命令tee相同的功能
 */

int main(int argc, char* argv[])
{
	int fd, file_pipe[2], stdout_pipe[2];
	ssize_t nspl;

	if (argc != 2)
		err_quit("usage: %s <file>", basename(argv[0]));

	if ((fd = open(argv[1], O_CREAT | O_TRUNC | O_WRONLY, DEFAULT_FMODE)) == -1)
		err_sys("open error");
	if (pipe(file_pipe) == -1)
		err_sys("pipe error");
	if (pipe(stdout_pipe) == -1)
		err_sys("pipe error");

	/* 若不清除STDOUT_FILENO的O_APPEND文件状态，则最后一个splice()函数
	   将会返回EINVAL错误。通过查看splice的man手册，可以发现造成这一错误
	   的一个原因就是描述符的O_APPEND文件状态标志被设置 */
	if (clr_fl(STDOUT_FILENO, O_APPEND) == -1)
		err_sys("clr_fl error");
	while ((nspl = splice(STDIN_FILENO, NULL, stdout_pipe[1], NULL,
		32768, SPLICE_F_MORE | SPLICE_F_MOVE)) > 0) {
		if (tee(stdout_pipe[0], file_pipe[1], nspl, SPLICE_F_NONBLOCK) == -1)
			err_sys("tee error");

		if (splice(file_pipe[0], NULL, fd, NULL, nspl, SPLICE_F_MORE | SPLICE_F_MOVE) == -1)
			err_sys("splice error");
		if (splice(stdout_pipe[0], NULL, STDOUT_FILENO, NULL, nspl,
			SPLICE_F_MORE | SPLICE_F_MOVE) == -1)
			err_sys("splice error");
	}
	if (nspl == -1)
		err_sys("splice error");
	exit(EXIT_SUCCESS);
}

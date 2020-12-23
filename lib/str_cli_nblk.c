#include "MyUNP.h"


/**
 * 使用非阻塞I/O方式+select函数实现的str_cli()函数再版，实际上
 * 它的性能和str_cli2（阻塞+select，实现更为简单）函数相近，但
 * 我感觉可以能比它差一点 */
void str_cli_nblk(int sockfd, FILE* fp) {
	char* sndoptr, * sndiptr, * rcvoptr, * rcviptr;
	char sndbuf[MAXLINE], rcvbuf[MAXLINE];
	int maxfdp1, stdineof, infd;
	ssize_t nread, nwrite, n;
	fd_set rset, wset;

	/* 设置非阻塞I/O方式 */
	infd = fileno(fp);
	if (set_fl(sockfd, O_NONBLOCK) == -1)
		err_sys("set_fl error");
	if (set_fl(infd, O_NONBLOCK) == -1)
		err_sys("set_fl error");
	if (set_fl(STDOUT_FILENO, O_NONBLOCK) == -1)
		err_sys("set_fl error");
	sndoptr = sndiptr = sndbuf;
	rcvoptr = rcviptr = rcvbuf;
	stdineof = 0;

	maxfdp1 = MAX(MAX(infd, STDOUT_FILENO), sockfd) + 1;
	for (;;) {
		FD_ZERO(&rset);
		FD_ZERO(&wset);
		if (stdineof == 0 && sndiptr < &sndbuf[MAXLINE])
			FD_SET(infd, &rset);
		if (rcviptr < &rcvbuf[MAXLINE])
			FD_SET(sockfd, &rset);
		if (sndoptr < sndiptr)
			FD_SET(sockfd, &wset);
		if (rcvoptr < rcviptr)
			FD_SET(STDOUT_FILENO, &wset);

		if (select(maxfdp1, &rset, &wset, NULL, NULL) == -1)
			err_sys("select error");

		/* 读stdin(fp)事务 */
		if (FD_ISSET(infd, &rset)) {
			if ((nread = read(infd, sndiptr, &sndbuf[MAXLINE] - sndiptr)) == -1) {
				if (errno != EWOULDBLOCK)
					err_sys("read error");
			}
			else if (nread == 0) {
#ifdef DEBUG
				fprintf(stderr, "%s: EOF on stdin\n", currtime_p("%T"));
#endif
				stdineof = 1;
				if (sndoptr == sndiptr)
					if (shutdown(sockfd, SHUT_WR) == -1)
						err_sys("shutdown error");
			}
			else {
#ifdef DEBUG
				fprintf(stderr, "%s: read %ld bytes from stdin\n", 
					currtime_p("%T"), nread);
#endif
				sndiptr += nread;
				FD_SET(sockfd, &wset);
			}
		}
		/* 读sockfd事务 */
		if (FD_ISSET(sockfd, &rset)) {
			if ((nread = read(sockfd, rcviptr, &rcvbuf[MAXLINE] - rcviptr)) == -1) {
				if (errno != EWOULDBLOCK)
					err_sys("read error");
			}
			else if (nread == 0) {
#ifdef DEBUG
				fprintf(stderr, "%s: EOF on socket\n", currtime_p("%T"));
#endif
				if (stdineof) return;
				else err_quit("str_cli_nblk: server terminated prematurely");
			}
			else {
#ifdef DEBUG
				fprintf(stderr, "%s: read %ld bytes from socket\n",
					currtime_p("%T"), nread);
#endif
				rcviptr += nread;
				FD_SET(STDOUT_FILENO, &wset);
			}
		}

		/* 写stdout事务 */
		if (FD_ISSET(STDOUT_FILENO, &wset) && (n = rcviptr - rcvoptr) > 0) {
			if ((nwrite = write(STDOUT_FILENO, rcvoptr, n)) == -1) {
				if (errno != EWOULDBLOCK)
					err_sys("write error");
			}
			else {
#ifdef DEBUG
				fprintf(stderr, "%s: wrote %ld bytes to stdout\n",
					currtime_p("%T"), nwrite);
#endif
				rcvoptr += nwrite;
				if (rcvoptr == rcviptr)
					rcviptr = rcvoptr = rcvbuf;
			}
		}
		/* 写sockfd事务 */
		if (FD_ISSET(sockfd, &wset) && (n = sndiptr - sndoptr) > 0) {
			if ((nwrite = write(sockfd, sndoptr, n)) == -1) {
				if (errno != EWOULDBLOCK)
					err_sys("write error");
			}
			else {
#ifdef DEBUG
				fprintf(stderr, "%s: wrote %ld bytes to sockfd\n",
					currtime_p("%T"), nwrite);
#endif
				sndoptr += nwrite;
				if (sndoptr == sndiptr) {
					sndoptr = sndiptr = sndbuf;
					if (stdineof)
						if (shutdown(sockfd, SHUT_WR) == -1)
							err_sys("shutdown error");
				}
			}
		}
	}
}
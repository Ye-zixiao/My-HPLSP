#include "MyUNP.h"


int main(int argc, char* argv[])
{
	int listenfd, connfd, pipefd[2];
	struct sockaddr* cliaddr;
	socklen_t clilen, addrlen;
	ssize_t nsp;
	
	if (argc == 2)
		listenfd = tcp_listen(NULL, argv[1], &addrlen);
	else if (argc == 3)
		listenfd = tcp_listen(argv[1], argv[2], &addrlen);
	else
		err_quit("usage: %s [host/ip] <serv/port>", basename(argv[0]));

	if ((cliaddr = malloc(addrlen)) == NULL)
		err_sys("malloc error");
	if (pipe(pipefd) == -1)
		err_sys("pipe error");

	for (;;) {
		clilen = addrlen;
		if ((connfd = accept(listenfd, cliaddr, &clilen)) == -1)
			err_sys("accept error");
		printf("%s: new connection from %s\n", currtime("%T"),
			sock_ntop(cliaddr, clilen));

		if (splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE) == -1)
			err_sys("splice error");
		if (splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE) == -1)
			err_sys("splice error");

		while ((nsp = splice(connfd, NULL, pipefd[1], NULL, 32768, SPLICE_F_MOVE | SPLICE_F_MORE)) > 0)
			if (splice(pipefd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE) == -1)
				err_sys("splice error");
		if (nsp == -1)
			err_sys("splice error");

		if (close(connfd) == -1)
			err_sys("close error");
	}
}
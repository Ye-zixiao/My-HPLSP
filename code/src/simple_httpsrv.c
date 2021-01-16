#include "MyUNP.h"
#include <sys/stat.h>

/**
 * 搭建简易的HTTP服务器：
 * 不要求客户端发送HTTP报文，只需要发送过来待读取的文件名，
 * 并试着使用writev()集中写的方式将HTTP的头部和实际文件数据
 * 一同打包发送给客户。个人感觉游双书上的代码不是很干净利落
 */

#define BUFFER_SIZE 1024

#define HTTP_ENDSTR "\r\n"
#define HTTP_OKSTATUS "HTTP/1.1 200 OK\r\n"
#define HTTP_BADSTATUS "HTTP/1.1 500 Internal server error\r\n"
#define HTTP_CONTENTLEN "Content-Length: %ld\r\n"


int main(int argc, char* argv[])
{
	char headbuf[BUFFER_SIZE], fname[SBUFSIZE];
	int fd, listenfd, connfd, headlen;
	socklen_t clilen, addrlen;
	struct sockaddr* cliaddr;
	char* filebuf = NULL;
	struct stat statbuf;
	struct iovec iov[2];
	ssize_t nread;

	if (argc == 3)
		listenfd = tcp_listen(argv[1], argv[2], &addrlen);
	else if (argc == 2)
		listenfd = tcp_listen(NULL, argv[1], &addrlen);
	else
		err_quit("usage: %s [host/ip] <serv/port>", basename(argv[0]));

	if ((cliaddr = malloc(addrlen)) == NULL)
		err_sys("malloc error");

	for (;;) {
		clilen = addrlen;
		if ((connfd = accept(listenfd, cliaddr, &clilen)) == -1)
			err_sys("accept error");
		printf("%s: new connection from %s\n", currtime("%T"),
			sock_ntop(cliaddr, clilen));

		//从客户请求中读取待读取文件名
		if ((nread = read(connfd, fname, SBUFSIZE)) == -1)
			err_sys("read error");
		fname[nread] = 0;

		/* 验证用户对文件的权限，若允许则读取到指定的缓冲区中，
			并封装好HTTP报文并发送给客户 */
		if (stat(fname, &statbuf) != -1 &&
			S_ISREG(statbuf.st_mode) && statbuf.st_mode & S_IROTH) {
			if ((filebuf = malloc(sizeof(statbuf.st_size + 1))) == NULL)
				err_sys("malloc error");
			if ((fd = open(fname, O_RDONLY)) == -1)
				err_sys("open error");
			if ((nread = read(fd, filebuf, statbuf.st_size)) == -1)
				err_sys("read error");
			if (close(fd) == -1)
				err_sys("close error");

			/* 向客户端发送状态为200 OK的HTTP报文 */
			headlen = snprintf(headbuf, BUFFER_SIZE - 1, HTTP_OKSTATUS);
			headlen += snprintf(headbuf + headlen, BUFFER_SIZE - 1 - headlen,
				HTTP_CONTENTLEN, statbuf.st_size);
			headlen += snprintf(headbuf + headlen, BUFFER_SIZE - 1 - headlen,
				HTTP_ENDSTR);
			iov[0].iov_base = headbuf;
			iov[0].iov_len = headlen;
			iov[1].iov_base = filebuf;
			iov[1].iov_len = statbuf.st_size;
			if (writev(connfd, iov, 2) != (headlen + statbuf.st_size))
				err_sys("writev error");

			free(filebuf);
		}
		else {
			headlen = snprintf(headbuf, BUFFER_SIZE - 1, HTTP_BADSTATUS);
			headlen += snprintf(headbuf + headlen, BUFFER_SIZE - 1 - headlen, HTTP_ENDSTR);
			if (write(connfd, headbuf, headlen) != headlen)
				err_sys("write error");
		}

		if (close(connfd) == -1)
			err_sys("close error");
	}
}
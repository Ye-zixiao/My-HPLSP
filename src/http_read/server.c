#include "MyUNP.h"
#include "http_read.h"


int main(int argc, char* argv[])
{
    int read_idx, chk_idx, start_line, chkstate, result;
    socklen_t addrlen, clilen;
    struct sockaddr* cliaddr;
    int listenfd, connfd;
    char buf[BUFSIZE];
    ssize_t nread;

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], &addrlen);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], &addrlen);
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

        start_line = read_idx = chk_idx = 0;
        chkstate = CHECK_STATE_REQUESTLINE;
        while ((nread = read(connfd, buf + read_idx, BUFSIZE - read_idx)) > 0) {
            read_idx += nread;

            //分析缓冲区中的内容
            if ((result = parse_content(buf, &chk_idx, &chkstate, &read_idx, &start_line)) == GET_REQUEST) {
                if (write(connfd, szret[0], strlen(szret[0])) != strlen(szret[0]))
                    err_sys("write error");
                break;
            }
            else if (result != NO_REQUEST) {
                if (write(connfd, szret[1], strlen(szret[1])) != strlen(szret[1]))
                    err_sys("write error");
                break;
            }
        }
        if (nread == -1)
            err_sys("read error");
        else if (nread == 0)
            printf("remote client has closed the connection\n");
        if (close(connfd) == -1)
            err_sys("close error");
    }
}
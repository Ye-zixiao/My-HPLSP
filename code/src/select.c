#include "MyUNP.h"


int main(int argc, char* argv[])
{
    int listenfd, connfd;
    char buf[BUFSIZE];
    fd_set rset, eset;
    ssize_t nrecv;

    if (argc == 2)
        listenfd = tcp_listen(NULL, argv[1], NULL);
    else if (argc == 3)
        listenfd = tcp_listen(argv[1], argv[2], NULL);
    else
        err_quit("usage: %s [host/ip] <serv/port>", basename(argv[0]));

    FD_ZERO(&rset);
    FD_ZERO(&eset);
    if ((connfd = accept(listenfd, NULL, NULL)) == -1)
        err_sys("accept error");
    for (;;) {
        FD_SET(connfd, &rset);
        FD_SET(connfd, &eset);
        if (select(connfd + 1, &rset, NULL, &eset, NULL) == -1)
            err_sys("select error");
        
        if (FD_ISSET(connfd, &rset)) {
            if ((nrecv = read(connfd, buf, sizeof(buf))) == -1)
                err_sys("read error");
            else if (nrecv == 0)
                break;
            buf[nrecv] = 0;
            printf("get %ld bytes of normal data: %s\n", nrecv, buf);
        }
        if (FD_ISSET(connfd, &eset)) {
            if ((nrecv = recv(connfd, buf, sizeof(buf), MSG_OOB)) == -1)
                err_sys("recv error");
            else if (nrecv == 0)
                break;
            buf[nrecv] = 0;
            printf("get %ld bytes of oob data: %s\n", nrecv, buf);
        }
        
    }

    exit(EXIT_SUCCESS);
}
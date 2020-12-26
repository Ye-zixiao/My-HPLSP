#ifndef MY_UNPXFE34_H_
#define MY_UNPXFE34_H_

#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/poll.h>
#include <sys/epoll.h>

#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define INFTIM -1
#define BUFSIZE 4096
#define SBUFSIZE 64
#define MAXLINE BUFSIZE
#ifndef OPEN_MAX
#define OPEN_MAX 1024
#endif

#define DEFAULT_FMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
#define LISTENQ 1024 //最大客户排队连接数

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) > (b) ? (b) : (a))


/* 错误例程 */
extern int daemon_proc;
void debug(void);
void err_ret(const char* fmt, ...);
void err_cont(int error, const char* fmt, ...);
void err_sys(const char* fmt, ...);
void err_dump(const char* fmt, ...);
void err_exit(int error, const char* fmt, ...);
void err_msg(const char* fmt, ...);
void err_quit(const char* fmt, ...);



/* 守护进程初始化 */
int daemon_init(const char* pname, int facility);
void daemon_inetd(const char* pname, int facility);


/* 网络地址辅助函数  */
char* sock_ntop(const struct sockaddr* sockaddr, socklen_t addrlen);



/* 流式套接字指定字节的读写 */
ssize_t readline(int fd, void* buf, size_t maxlen);
ssize_t readline1(int fd, void* buf, size_t maxlen);
ssize_t readline_r(int fd, void* buf, size_t maxlen);
ssize_t readlinebuf(void** cptrptr);

#ifdef MSG_WAITALL
#define readn(fd, buf, nbytes)	recv(fd, buf, nbytes, MSG_WAITALL)
#define writen(fd, buf, nbytes)	send(fd, buf, nbytes, MSG_WAITALL)
#else
ssize_t readn(int fd, void* buf, size_t nbytes);
ssize_t writen(int fd, const void* buf, size_t nbytes);
#endif



/* 时间状态函数 */
const char* currtime(const char* fmt);
const char* currtime_p(const char* ignore);
char* currtime_r(char* buf, size_t maxlen, const char* fmt);



/* 自定义信号处理程序安装 */
typedef void Sigfunc(int);
Sigfunc* mysignal(int signo, Sigfunc* func);



/* 回射客户-服务器辅助函数 */
void str_echo(int sockfd);
void str_echo1(int sockfd);
void str_echo2(int sockfd);
void str_echo3(int sockfd);
void str_echo_r(int sockfd);
void str_cli(int sockfd, FILE* fp);
void str_cli1(int sockfd, FILE* fp);
void str_cli2(int sockfd, FILE* fp);
void str_cli3(int sockfd, FILE* fpin, FILE* fpout);
void str_clip(int sockfd, FILE* fp);
void str_clit(int sockfd, FILE* fp);
void str_cli_nblk(int sockfd, FILE* fp);

void sum_echo1(int sockfd);
void sum_echo2(int sockfd);
void sum_cli2(int sockfd, FILE* fp);

void dg_echo(int sockfd, struct sockaddr* cliaddr, socklen_t clilen);
void dg_echox(int sockfd, struct sockaddr* cliaddr, socklen_t clilen);
void dg_cli(int sockfd, FILE* fp, 
        const struct sockaddr* svaddr, socklen_t svlen);
void dg_clit0(int sockfd, FILE* fp,
        const struct sockaddr* svaddr, socklen_t svlen);
void dg_clit1(int sockfd, FILE* fp,
        const struct sockaddr* svaddr, socklen_t svlen);
void dg_clit2(int sockfd, FILE* fp,
        const struct sockaddr* svaddr, socklen_t svlen);
void dg_cli1(int sockfd, FILE* fp,
        const struct sockaddr* svaddr, socklen_t svlen);
void dg_cli2(int sockfd, FILE* fp, 
        const struct sockaddr* svaddr, socklen_t svlen);
void dg_clix(int sockfd, FILE* fp,
        const struct sockaddr* svaddr, socklen_t svlen);



/* 文件控制函数 */
int set_fd(int fd, int nflag);
int set_fl(int fd, int nflag);
int clr_fd(int fd, int cflag);
int clr_fl(int fd, int cflag);



/* 地址解析辅助函数 */
struct servent* getserv(const char* name_or_port, const char* protoname);
int gethost(struct in_addr* addrArr, int n, const char* name);
struct hostent* gethost1(const char* host);



/* 由getaddrinfo()函数派生出的辅助函数 */
struct addrinfo*
    host_serv(const char* host, const char* serv, int family, int socktype);
int tcp_connect(const char* host, const char* serv);
int tcp_listen(const char* host, const char* serv, socklen_t* addrlen);
int udp_client(const char* host, const char* serv, struct sockaddr** saptr, socklen_t* lenp);
int udp_connect(const char* host, const char* serv);
int udp_server(const char* host, const char* serv, socklen_t* lenp);



/* 高级I/O函数 */
int connect_timeo(int sockfd, const struct sockaddr* svaddr, socklen_t len, time_t nsec);
int readable_timeo(int sockfd, time_t nsec);
int writeable_timeo(int sockfd, time_t nsec);

ssize_t send_fd(int sockfd, void* vptr, size_t nbytes, int sendfd);
ssize_t recv_fd(int sockfd, void* vptr, size_t nbytes, int* recvfd);


/* 文件锁 */
int lock_reg(int fd, int cmd, int lock_type, off_t offset, int whence, int len);
int lock_test(int fd, int cmd, int lock_type, off_t offset, int whence, int len);
#define read_lock(fd, offset, whence, len)		\
        lock_reg((fd), F_SETLK, F_RDLCK, (offset), whence, (len))
#define readw_lock(fd, offset, whence, len)		\
        lock_reg((fd), F_SETLKW, F_RDLCK, (offset), whence, (len))
#define write_lock(fd, offset, whence, len)		\
        lock_reg((fd), F_SETLK, F_WRLCK, (offset), whence, (len))
#define writew_lock(fd, offset, whence, len)		\
        lock_reg((fd), F_SETLKW, F_WRLCK, (offset), whence, (len))
#define unlock(fd, offset, whence, len)			\
        lock_reg((fd), F_SETLK, F_UNLCK, (offset), whence, (len))
#define unlock1(fd, offset, whence, len)			\
        lock_reg((fd), F_SETLKW, F_UNLCK, (offset), whence, (len))


/* 非阻塞相关函数 */
int connect_nblk(int sockfd, 
        const struct sockaddr* svaddr, socklen_t svlen, time_t nsec);



/* 线程相关函数 */
int pthread_create_detached(pthread_t* thread, void* (*pf)(void*), void* arg);



/* 其他 */
void pr_cpu_time(void);
void web_child(int sockfd);
void web_child_r(int sockfd);
void web_child_r1(int sockfd);



#endif //!MY_UNPXFE34_H_

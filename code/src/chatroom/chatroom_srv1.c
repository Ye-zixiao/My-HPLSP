#include "MyUNP.h"
#include <sys/mman.h>

/**
 * 一个使用到共享内存的群聊服务器。当客户连接后，它会
 * fork出一个子进程专门处理它。并且期间接收到的数据都
 * 是存放在一个进程间共享内存段中，由父进程告知某些子
 * 进程它们需要群发哪个用户的消息。
 */

#define FALSE 0
#define TRUE 1
#define USER_LIMIT 5
#define FD_LIMIT 65535
#define MAX_EVENT_NUMBERS 1024
#define PROCESS_LIMIT 65535
#define SHM_NAME "/my_shm"

struct client_data {
    struct sockaddr_in cliaddr;
    int pipefd[2];
    int connfd;
    pid_t pid;
};


int shm_fd, user_count, sig_pipefd[2];
struct client_data* users = NULL;
char* shared_mem = NULL;
int* sub_proccess = NULL;
int stop_child = FALSE;


static void sig_handler(int signo) {
    int errno_save = errno;
    if (write(sig_pipefd[1], &signo, 1) != 1)
        err_sys("write error");
    errno = errno_save;
}

static void child_term_handler(int signo) {
    stop_child = TRUE;
}


int child_run(int index, struct client_data* users, char* shared_mem) {
    struct epoll_event events[MAX_EVENT_NUMBERS];
    int epfd, connfd, sockfd, pipefd, nret;
    ssize_t nread;

    if ((epfd = epoll_create(5)) == -1)
        err_sys("epoll_create error");
    connfd = users[index].connfd;
    pipefd = users[index].pipefd[1];
    add2epoll(epfd, connfd);
    add2epoll(epfd, pipefd);
    if (mysignal1(SIGTERM, child_term_handler) == SIG_ERR)
        err_sys("mysignal1 error");

    while (!stop_child) {
        if ((nret = epoll_wait(epfd, events, MAX_EVENT_NUMBERS, -1)) == -1) {
            if (errno == EINTR) continue;
            err_sys("epoll_wait error");
        }

        for (int i = 0; i < nret; ++i) {
            sockfd = events[i].data.fd;

            /* 接收来自客户发送过来的数据，存放到共享内存客户专属的缓冲区中，
                然后将客户的编号告诉父进程 */
            if (sockfd == connfd && (events[i].events & EPOLLIN)) {
                if ((nread = read(connfd, shared_mem + MBUFSIZE * index, MBUFSIZE - 1)) == -1) {
                    if (errno = EWOULDBLOCK) continue;
                    err_sys("read error");
                }
                else if (nread == 0) {
                    stop_child = TRUE;
                    break;
                }
                *(shared_mem + MBUFSIZE * index + nread) = 0;
                if (write(pipefd, &index, sizeof(index)) != sizeof(index))
                    err_sys("write error");
            }
            //父进程告知子进程需要读取哪个缓冲区数据并将其发送给当前进程服务的客户
            else if (sockfd == pipefd && (events[i].events & EPOLLIN)) {
                int client = 0;
                if ((nread = read(pipefd, &client, sizeof(client))) != sizeof(client)) {
                    if (errno == EWOULDBLOCK) continue;
                    err_sys("read error");
                }
                else if (nread == 0) {
                    stop_child = TRUE;
                    break;
                }
                char* cptr = shared_mem + MBUFSIZE * client;
                if (write(connfd, cptr, strlen(cptr)) != strlen(cptr))
                    err_sys("write error");
            }
        }
    }

    close(connfd);
    close(pipefd);
    close(epfd);
    exit(EXIT_SUCCESS);
}


int main(int argc, char* argv[])
{
    struct epoll_event events[MAX_EVENT_NUMBERS];
    int stop_server = FALSE, terminate = FALSE;
    int listenfd, sockfd, connfd, epfd, nret;
    struct sockaddr_in cliaddr;
    socklen_t clilen;

    if (argc < 2)
        err_quit("usage: %s <serv/port>", basename(argv[0]));
    
    listenfd = tcp_listen(NULL, argv[1], NULL);
    if ((users = calloc(USER_LIMIT + 1, sizeof(struct client_data))) == NULL)
        err_sys("calloc error");
    if ((sub_proccess = calloc(PROCESS_LIMIT, sizeof(int))) == NULL)
        err_sys("calloc error");
    for (int i = 0; i < PROCESS_LIMIT; ++i) sub_proccess[i] = -1;
    user_count = 0;

    if ((epfd = epoll_create(5)) == -1)
        err_sys("epoll_create error");
    add2epoll(epfd, listenfd);
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sig_pipefd) == -1)
        err_sys("socketpair error");
    setnonblocking(sig_pipefd[1]);
    add2epoll(epfd, sig_pipefd[0]);

    if (mysignal1(SIGCHLD, sig_handler) == SIG_ERR)
        err_sys("mysignal1 error");
    if (mysignal1(SIGTERM, sig_handler) == SIG_ERR)
        err_sys("mysignal1 error");
    if (mysignal1(SIGINT, sig_handler) == SIG_ERR)
        err_sys("mysignal1 error");
    if (mysignal1(SIGPIPE, SIG_IGN) == SIG_ERR)
        err_sys("mysignal1 error");

    //创建一个共享内存
    if ((shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666)) == -1)
        err_sys("shm_open error");
    if ((nret = ftruncate(shm_fd, USER_LIMIT * MBUFSIZE)) == -1)
        err_sys("ftruncate error");
    if ((shared_mem = mmap(NULL, USER_LIMIT * MBUFSIZE, PROT_READ | PROT_WRITE,
        MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
        err_sys("mmap error");
    close(shm_fd);

    while (!stop_server) {
        if ((nret = epoll_wait(epfd, events, MAX_EVENT_NUMBERS, -1)) == -1) {
            if (errno == EINTR) continue;
            err_sys("epoll_wait error");
        }

        for (int i = 0; i < nret; ++i) {
            sockfd = events[i].data.fd;

            if (sockfd == listenfd && (events[i].events & EPOLLIN)) {
                clilen = sizeof(cliaddr);
                if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1)
                    err_sys("accept error");
                printf("%s: new users(%s fd %d) coming in...\n", currtime("%T"),
                    sock_ntop((const struct sockaddr*)&cliaddr, clilen), connfd);

                if (user_count >= USER_LIMIT) {
                    char msg[] = "too many users";
                    if (write(connfd, msg, strlen(msg)) != strlen(msg))
                        err_sys("write error");
                    close(connfd);
                    err_msg(msg);
                    continue;
                }
                users[user_count].cliaddr = cliaddr;
                users[user_count].connfd = connfd;
                if (socketpair(AF_UNIX, SOCK_STREAM, 0, users[user_count].pipefd) == -1)
                    err_sys("socketpair error");

                pid_t pid;
                if ((pid = fork()) == -1)
                    err_sys("fork error");
                else if (pid == 0) {
                    //子进程
                    close(epfd);
                    close(listenfd);
                    close(users[user_count].pipefd[0]);
                    close(sig_pipefd[0]);
                    close(sig_pipefd[1]);
                    child_run(user_count, users, shared_mem);
                    munmap(shared_mem, USER_LIMIT * MBUFSIZE);
                    exit(EXIT_SUCCESS);
                }
                else {
                    //父进程
                    close(connfd);
                    close(users[user_count].pipefd[1]);
                    add2epoll(epfd, users[user_count].pipefd[0]);
                    users[user_count].pid = pid;
                    sub_proccess[pid] = user_count;
                    user_count++;
                }
            }
            else if (sockfd == sig_pipefd[0] && (events[i].events & EPOLLIN)) {
                char signals[1024];
                ssize_t nread;

                if ((nread = read(sockfd, signals, sizeof(signals))) <= 0)
                    continue;
                for (int i = 0; i < nread; ++i) {
                    switch (signals[i]) {
                    case SIGCHLD: {
                        pid_t pid;
                        int stat;
                        while ((pid = waitpid(-1, &stat, WNOHANG)) > 0) {
                            /* 先处理sub_proccess，然后处理users用户数据 */
                            int del_user = sub_proccess[pid];
                            sub_proccess[pid] = -1;
                            if (del_user<0 || del_user>USER_LIMIT)
                                continue;
                            if (epoll_ctl(epfd, EPOLL_CTL_DEL, users[del_user].pipefd[0], NULL) == -1)
                                err_sys("epoll_ctl error");
                            close(users[del_user].pipefd[0]);
                            //移动users最后一个元素填充这个位置，然后重新更新相关数据
                            users[del_user] = users[--user_count];
                            sub_proccess[users[del_user].pid] = del_user;
                            printf("a client left\n");
                        }
                        //只有将所有的子进程都杀光了才能真正的退出
                        if (terminate && user_count == 0)
                            stop_server = TRUE;
                        break; 
                    }
                    case SIGTERM:case SIGINT: {
                        printf("\nserver is closing...\n");
                        if (user_count == 0) {
                            stop_server = TRUE;
                            break;
                        }
                        for (int i = 0; i < user_count; ++i)
                            kill(users[i].pid, SIGTERM);
                        terminate = TRUE;
                        break;
                    }
                    default: break;
                    }
                }
            }
            else if (events[i].events & EPOLLIN) {
                int child = -1, ret;
                /* 因为子进程在终止之前会关闭写端管道描述符，导致父进程的管道读端描述符可读，
                    但读取时会发现得到却是0。不过这里不关闭它，而是在收到SIGCHLD信号时再关闭 */
                if ((ret = read(sockfd, &child, sizeof(child))) == -1) {
                    if (errno == EWOULDBLOCK) continue;
                    err_sys("read error");
                }
                else if (ret == 0) continue;

                for (int i = 0; i < user_count; i++) {
                    if (users[i].pipefd[0] != sockfd) {
                        printf("send data to child accross pipe\n");
                        if (write(users[i].pipefd[0], &child, sizeof(child)) == -1)
                            err_sys("write error");
                    }
                }
            }
        }
    }

    //相关的描述符其实会被自动关闭
    free(users);
    free(sub_proccess);
    exit(EXIT_SUCCESS);
}
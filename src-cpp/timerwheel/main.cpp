extern "C" {
#include "MyUNP.h"
}
#include "TimerWheel.h"
#include <iostream>


#define MAX_EVENT_NUMS 1024
#define FD_LIMIT 65535
#define TIMEOUT 16

namespace {
    Client_Data users[FD_LIMIT];
    TimerWheel timer_wheel;
    int epfd;
}


/* 回调函数，用来关闭连接并将相关描述符从epoll内核事件表中移除 */
void callback_func(Client_Data* user_data) {
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, user_data->sockfd, NULL) == -1)
        err_sys("epoll_ctl error");
    if (close(user_data->sockfd) == -1)
        err_sys("close error");
    std::cout << "close fd " << user_data->sockfd << std::endl;
}



int main(int argc, char* argv[])
{
    int sockfd, connfd, listenfd, nret;
    epoll_event events[MAX_EVENT_NUMS];
    time_t start, end, timeout;
    sockaddr_in cliaddr;
    ssize_t nrecv;

    if (argc != 2)
        err_quit("usage: %s <serv/port>", basename(argv[0]));
    
    listenfd = tcp_listen(NULL, argv[1], NULL);
    if ((epfd = epoll_create(10)) == -1)
        err_sys("epoll_create error");
    add2epoll(epfd, listenfd);

    timeout = TimerWheel::SI * 1000;
    for (; ;) {
        start = time(NULL);
        if ((nret = epoll_wait(epfd, events, MAX_EVENT_NUMS, timeout)) == -1) {
            if (errno == EINTR) continue;
            err_sys("epoll_wait error");
        }
        /* epoll_wait因超时而返回0，故此时处理定时事件 */
        else if (nret == 0) {
            timeout = TimerWheel::SI * 1000;
            timer_wheel.tick();
            continue;
        }
        
        for (int i = 0; i < nret; ++i) {
            sockfd = events[i].data.fd;

            /* 有新的连接请求到来 */
            if (sockfd == listenfd) {
                socklen_t clilen = sizeof(sockaddr_in);
                if ((connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &clilen)) == -1)
                    err_sys("accept error");
                std::cout << currtime("%T") << ": new connection from "
                    << sock_ntop((const struct sockaddr*)&cliaddr, clilen)<< std::endl;

                //更新用户信息users并添加定时器到定时器链表中
                add2epoll(epfd, connfd);
                users[connfd].cliaddr = cliaddr;
                users[connfd].sockfd = connfd;
                TWTimer* timer = timer_wheel.add_timer(TIMEOUT);
                timer->callback = callback_func;
                timer->user_data = &users[connfd];
                users[connfd].timer = timer;
            }
            else if (events[i].events & EPOLLIN) {
                TWTimer* timer = users[sockfd].timer;

                while ((nrecv = read(sockfd, users[sockfd].buf, BUFFER_SIZE - 1)) > 0) {
                    users[sockfd].buf[nrecv] = 0;
                    std::cout << "get " << nrecv << " bytes of client data " << users[sockfd].buf
                        << " from " << sockfd << std::endl;
                    if (timer) timer_wheel.adjust_timer(timer, TIMEOUT);
                }
                //连接被关闭或数据读取发生了错误
                if (nrecv == 0 || (nrecv == -1 && errno != EWOULDBLOCK)) {
                    callback_func(&users[sockfd]);
                    if (timer) timer_wheel.del_timer(timer);
                }
            }

            /* 能到达此处说明epoll_wait并不是因为超时而返回的，
                故在重新迭代前需要调整超时值 */
            end = time(NULL);
            timeout -= (end - start) * 1000;
            if (time <= 0) timeout = TimerWheel::SI * 1000;
        }
    }
}

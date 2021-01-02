#ifndef TIMERLIST_H_
#define TIMERLIST_H_

#include <time.h>
#include <netinet/in.h>

#define BUFFER_SIZE 64

struct Timer;

/* 用户信息 */
struct Client_Data {
    char buf[BUFFER_SIZE];
    sockaddr_in cliaddr;
    int sockfd;
    Timer* timer;
};

/* 定时器类 */
struct Timer {
    void (*callback)(Client_Data*);
    Timer* prev, * next;
    Client_Data* client_data;
    time_t expire;

    Timer():prev(nullptr),next(nullptr){}
};


/* 定时器升序链表 */
class TimerList {
public:
    TimerList() :head(nullptr), tail(nullptr) {}
    ~TimerList();

    void add_timer(Timer* timer);
    void del_timer(Timer* timer);
    void adjust_timer(Timer* timer);
    void tick();

private:
    void add_timer(Timer* timer, Timer* list_head);

    Timer* head, * tail;
};


#endif // !TIMERLIST_H_
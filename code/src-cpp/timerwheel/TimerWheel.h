#ifndef TIMERWHEEL_H_
#define TIMERWHEEL_H_

#include <time.h>
#include <netinet/in.h>

#define BUFFER_SIZE 64
//#define DEBUG

struct TWTimer;

/* 用户数据 */
struct Client_Data {
    char buf[BUFFER_SIZE];
    sockaddr_in cliaddr;
    TWTimer* timer;
    int sockfd;
};


/* 定时器类 */
struct TWTimer {
    TWTimer(int rot,int ts):
        rotation(rot),time_slot(ts){}

    TWTimer* prev = nullptr, * next = nullptr;
    void (*callback)(Client_Data*);
    Client_Data* user_data;
    int rotation, time_slot;
};


/* 时间轮类，本质上就是一个哈希表 */
class TimerWheel {
public:
    TimerWheel();
    ~TimerWheel();

    TWTimer* add_timer(int timeout);
    void adjust_timer(TWTimer* timer, int timeout);
    void del_timer(TWTimer* timer);
    void tick();

public:
    static constexpr int SI = 2;    //时间轮的槽间隔

private:
    static constexpr int N = 60;    //时间轮槽数
    TWTimer* slots[N];
    int currslot;
};


#endif // !TIMERWHEEL_H_

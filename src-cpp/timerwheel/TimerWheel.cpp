#include "TimerWheel.h"
#include <algorithm>
#include <iostream>


TimerWheel::TimerWheel() :
    currslot(0) {
    std::fill(slots, slots + N, nullptr);
}


TimerWheel::~TimerWheel() {
    for (int i = 0; i < N; ++i)
        for (TWTimer* tmp = slots[i]; tmp; tmp = slots[i]) {
            slots[i] = tmp->next;
            delete tmp;
        }
}


/* 添加定时器插入到时间轮相应槽位链表的头部 */
TWTimer* TimerWheel::add_timer(int timeout) {
    if (timeout <= 0) return nullptr;

    int rot = timeout / (N * SI);
    int ts = (currslot + timeout / SI) % N;
    TWTimer* timer = new TWTimer(rot, ts);
    if (!slots[ts]) {
        std::cout << "add timer: rotation " << rot << ", ts " << ts
            << ", currslot " << currslot << std::endl;
        slots[ts] = timer;
    }
    else {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }
    return timer;
}


void TimerWheel::adjust_timer(TWTimer* timer, int timeout) {
    if (!timer) return;

    int ts = timer->time_slot;
    if (timer == slots[ts]) {
        slots[ts] = slots[ts]->next;
        if (slots[ts]) slots[ts]->prev = nullptr;
    }
    else {
        timer->prev->next = timer->next;
        if (timer->next) timer->next->prev = timer->prev;
    }

    int rot = timeout / (N * SI);
    ts = (currslot + timeout / SI) % N;
    timer->time_slot = ts;
    timer->rotation = rot;

    if (!slots[ts]) {
#ifdef DEBUG
        std::cout << "add timer, rotation " << rot << ", ts " << ts
            << ", currslot " << currslot << std::endl;
#endif
        slots[ts] = timer;
    }
    else {
        timer->next = slots[ts];
        slots[ts]->prev = timer;
        slots[ts] = timer;
    }
}


/* 删除指定的定时器 */
void TimerWheel::del_timer(TWTimer* timer) {
    if (!timer) return;

    int ts = timer->time_slot;
    if (timer == slots[ts]) {
        slots[ts] = slots[ts]->next;
        if (slots[ts]) slots[ts]->prev = nullptr;
    }
    else {
        timer->prev->next = timer->next;
        if (timer->next) timer->next->prev = timer->prev;
    }
    delete timer;
}


/* 每隔SI秒滴答一下，处理时间轮当前槽位链表上的超时事件 */
void TimerWheel::tick() {
    TWTimer* tmp = slots[currslot];
    
#ifdef DEBUG
    std::cout << "current slot: " << currslot << std::endl;
#endif
    while (tmp) {
        std::cout << "tick the timer once" << std::endl;
        if (tmp->rotation > 0) {
            tmp->rotation--;
            tmp = tmp->next;
        }
        else {
            tmp->callback(tmp->user_data);
            if (tmp == slots[currslot]) {
#ifdef DEBUG
                std::cout << "delete header in currslot" << std::endl;
#endif
                slots[currslot] = tmp->next;
                delete tmp;
                if (slots[currslot]) slots[currslot]->prev = nullptr;
                tmp = slots[currslot];
            }
            else {
                TWTimer* tmp1 = tmp->next;
                tmp->prev->next = tmp1;
                if (tmp1) tmp1->prev = tmp->prev;
                delete tmp;
                tmp = tmp1;
            }
        }
    }
    currslot = (currslot + 1) % N;
}

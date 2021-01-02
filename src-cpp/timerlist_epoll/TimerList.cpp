#include "TimerList.h"
#include <iostream>


TimerList::~TimerList() {
    for (Timer* tmp = head; tmp; tmp = head) {
        head = head->next;
        delete tmp;
    }
}


//添加定时器
void TimerList::add_timer(Timer* timer) {
    if (!timer)return;
    if (!head) { head = tail = timer; return; }
    if (timer->expire < head->expire) {
        timer->next = head;
        head = timer;
        return;
    }
    add_timer(timer,head);
}


//添加定时器到定时器链表首结点之后的某个位置
void TimerList::add_timer(Timer* timer, Timer* lst_head) {
    Timer* prev = lst_head, * tmp = prev->next;
    for (; tmp; prev = tmp, tmp = tmp->next) {
        if (timer->expire < tmp->expire) {
            prev->next = timer;
            timer->next = tmp;
            tmp->prev = timer;
            timer->prev = prev;
            break;
        }
    }
    if (!tmp) {
        prev->next = timer;
        timer->next = nullptr;
        timer->prev = prev;
        tail = timer;
    }
}


//删除定时器
void TimerList::del_timer(Timer* timer) {
    if (!timer) return;
    if (timer == head && timer == tail) {
        delete timer;
        head = nullptr;
        tail = nullptr;
        return;
    }
    else if (timer == head) {
        head = head->next;
        head->prev = nullptr;
        delete timer;
        return;
    }
    else if (timer == tail) {
        tail = tail->prev;
        tail->next = nullptr;
        delete timer;
        return;
    }
    timer->prev->next = timer->next;
    timer->next->prev = timer->prev;
    delete timer;
}


/* SIGALRM信号每递交一次就在信号处理程序中调用一次该函数，以处理
调用定时器链表中已到期的定时器。这里定时器并不是真的“准时”，它是每
隔一个固定时间间隔触发一次SIGALRM，然后处理掉这段时间内的定时器，
并不会为每一个定时器都触发一次SIGALRM信号   */
void TimerList::tick() {
    if (!head) return;
    std::cout << "timer tick" << std::endl;

    time_t cur = time(NULL);
    for (Timer* tmp = head; tmp && cur >= tmp->expire; tmp = head) {
        tmp->callback(tmp->client_data);
        head = head->next;
        if (head) head->prev = nullptr;
        delete tmp;
    }
}


//向后调整定时器
void TimerList::adjust_timer(Timer* timer) {
    if (!timer) return;

    Timer* tmp = timer->next;
    if (!tmp || timer->expire < tmp->expire)
        return;
    if (timer == head) {
        head = head->next;
        head->prev = nullptr;
        timer->next = nullptr;
        add_timer(timer, head);
    }
    else {
        timer->next->prev = timer->prev;
        timer->prev->next = timer->next;
        add_timer(timer, timer->next);
    }
}
#include "lst_timer.h"


sort_time_lst::~sort_time_lst() {
	for (util_timer* tmp = head; tmp;) {
		head = head->next;
		delete tmp;
		tmp = head;
	}
}


//添加定时器
void sort_time_lst::add_timer(util_timer* timer) {
	if (!timer) return;
	if (!head) { head = tail = timer; return; }
	if (timer->expire < head->expire) {
		timer->next = head;
		head = timer;
		return;
	}
	add_timer(timer, head);
}


//添加定时器辅助函数
void sort_time_lst::add_timer(util_timer* timer, util_timer* lst_head) {
	util_timer* prev = lst_head, * tmp = prev->next;
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
void sort_time_lst::del_timer(util_timer* timer) {
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


/* SIGALRM信号没被递交一次就在信号处理程序中调用一次该函数，
	以处理调用定时器链表中已到期的定时器   */
void sort_time_lst::tick() {
	if (!head) return;
	std::cout << "timer tick" << std::endl;

	time_t cur = time(NULL);
	for (util_timer* tmp = head; tmp && cur >= tmp->expire; tmp = head) {
		tmp->cb_func(tmp->user_data);
		head = head->next;
		if (head) head->prev = nullptr;
		delete tmp;
	}
}


//向后调整定时器
void sort_time_lst::adjust_timer(util_timer* timer) {
	if (!timer) return;

	util_timer* tmp = timer->next;
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
#ifndef LST_TIMERXFS2_H_
#define LST_TIMERXFS2_H_


#include <time.h>
#include <netinet/in.h>
#include <iostream>

#define BUFFER_SIZE 64

class util_timer;

/* 用户数据结构 */
struct client_data {
	int sockfd;
	util_timer* timer;
	char buf[BUFFER_SIZE];
	sockaddr_in addr;
};


/* 定时器类 */
class util_timer {
public:
	time_t expire;
	util_timer* prev;
	util_timer* next;
	client_data* user_data;
	void (*cb_func)(client_data*);

	util_timer():prev(nullptr),next(nullptr){}
};


/* 定时器链表 */
class sort_time_lst {
	util_timer* head, * tail;

public:
	sort_time_lst():head(nullptr),tail(nullptr){}
	~sort_time_lst();

	void add_timer(util_timer* timer);
	void adjust_timer(util_timer* timer);
	void del_timer(util_timer* timer);
	void tick();
	
private:
	void add_timer(util_timer* timer, util_timer* lst_head);
};


#endif // !LST_TIMERXFS2_H_

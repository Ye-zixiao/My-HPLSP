#ifndef TIMERHEAP_H_
#define TIMERHEAP_H_

#include <vector>
#include <queue>
#include <netinet/in.h>

#define BUFFER_SIZE 64
//#define DEBUG 1

struct HTimer;

//用户数据
struct Client_Data {
	char buf[BUFFER_SIZE];
	sockaddr_in cliaddr;
	HTimer* timer;
	int sockfd;
};


//定时器类
struct HTimer {
	typedef bool (*comp_type)(HTimer* const&, HTimer* const&);

	//HTimer(time_t delay, Client_Data* ud = nullptr,
	//	void(*pf)(Client_Data*) = nullptr);

	void(*callback)(Client_Data*);
	Client_Data* user_data;
	time_t expire;
};


//定时器堆
class TimerHeap {
public:
	TimerHeap();
	~TimerHeap();

	bool empty() const { return pq.empty(); }
	void add_timer(HTimer* timer);
	void del_timer(HTimer* timer);
	HTimer* top_timer() const;
	void pop_timer();
	void tick();

private:
	std::priority_queue<HTimer*, std::vector<HTimer*>,
		HTimer::comp_type> pq;
};


#endif // !TIMERHEAP_H_

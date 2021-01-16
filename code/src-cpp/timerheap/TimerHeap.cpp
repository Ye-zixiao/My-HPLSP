#include "TimerHeap.h"
#include <iostream>
#include <ctime>


bool HTimer_Comp(HTimer* const& lhs, HTimer* const& rhs) {
	return lhs->expire > rhs->expire;
}


TimerHeap::TimerHeap():pq(HTimer_Comp){}

TimerHeap::~TimerHeap() {
	for (HTimer* tmp; !pq.empty();) {
		tmp = pq.top();
		pq.pop();
		delete tmp;
	}
}

void TimerHeap::add_timer(HTimer* timer) {
	if(timer) pq.push(timer);
}

void TimerHeap::del_timer(HTimer* timer) {
	if (timer) timer->callback = nullptr;
}

HTimer* TimerHeap::top_timer() const {
	return pq.top();
}

void TimerHeap::pop_timer() {
	HTimer* tmp = pq.top();	
	pq.pop();
	delete tmp;
}

void TimerHeap::tick() {
#ifdef DEBUG
	std::cout << "time tick" << std::endl;
#endif // DEBUG
	for (HTimer* tmp; !pq.empty();) {
		tmp = pq.top();
		if (tmp->expire > time(NULL)) break;
		if (tmp->callback) tmp->callback(tmp->user_data);
		pq.pop();
	}
}
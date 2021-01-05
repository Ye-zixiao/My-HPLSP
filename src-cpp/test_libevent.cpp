#include <event.h>
#include <signal.h>
#include <iostream>


void signal_callback(int fd, short event, void* argc) {
	struct event_base* base = (event_base*)argc;
	struct timeval  delay = { 2,0 };
	std::cout << "\nCaught an interrupt signal; exiting cleanly in two seconds..." << std::endl;
	event_base_loopexit(base, &delay);
}


void timeout_callback(int fd, short event, void* argc) {
	std::cout << "timeout" << std::endl;
}


int main() {
	struct event_base* base = event_init();

	//添加信号事件
	struct event* signal_event = evsignal_new(base, SIGINT, signal_callback, base);
	event_add(signal_event, NULL);

	timeval tv = { 1,0 };
	//添加定时事件
	struct event* timeout_event = evtimer_new(base, timeout_callback, NULL);
	//struct event* timeout_event = event_new(base, -1,
	//		EV_TIMEOUT | EV_PERSIST, timeout_callback, NULL);
	event_add(timeout_event, &tv);

	event_base_dispatch(base);
	
	event_free(timeout_event);
	event_free(signal_event);
	event_base_free(base);
}
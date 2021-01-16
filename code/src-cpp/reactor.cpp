#include "MyUNP.h"
#include <iostream>
#include <unordered_map>


//解多路复用器
class Event_Demultiplexer {
public:
	Event_Demultiplexer():
		epfd(epoll_create(5)){}
	~Event_Demultiplexer() { close(epfd); }

	int register_event(struct epoll_event& ev) {
		return epoll_ctl(epfd, EPOLL_CTL_ADD, ev.data.fd, &ev);
	}

	int remove_event(int fd) {
		return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
	}

	template<int max_events>
	int wait(std::array<struct epoll_event, max_events>& event_arr,int timeout) {
		return epoll_wait(epfd, event_arr.data(), max_events, timeout);
	}

private:
	int epfd;
};


//事件处理器，这里并没有采用继承的方式实现
class Event_Handler {
public:
	using callback_type = void(*)(int);

	Event_Handler(int fd,callback_type cb):
		eventfd(fd),callback(cb){}
	virtual ~Event_Handler() = default;

	int get_handle() const { return eventfd; }
	void handle_event() { callback(eventfd); }

protected:
	int eventfd;
	callback_type callback;
};


//反应器
class Reactor {
public:
	
	void register_handler(Event_Handler* handler) {
		struct epoll_event ev;
		ev.data.fd = handler->get_handle();
		ev.events = EPOLLIN | EPOLLET;
		demultiplexer.register_event(ev);
		handlers[ev.data.fd] = handler;
	}


	void remove_handler(int fd) {
		demultiplexer.remove_event(fd);
		auto handler = handlers[fd];
		handlers.erase(fd);
		delete handler;
	}

	void handle_events() {
		std::array<struct epoll_event, 10> event_arrs;
		for (;;) {
			int nret = demultiplexer.wait<10>(event_arrs, -1);
			
			for (int i = 0; i < nret; ++i) {
				//分发处理。对于多线程而言应该会有一个发送到请求队列的过程
				handlers[event_arrs[i].data.fd]->handle_event();
			}
		}
	}
	
private:
	Event_Demultiplexer demultiplexer;
	std::unordered_map<int, Event_Handler*> handlers;
};


static Reactor reactor;

//I/O事件的回调函数
void callback_func(int sockfd) {
	char buf[MBUFSIZE];
	ssize_t nread;

	while ((nread = read(sockfd, buf, sizeof(buf) - 1)) > 0) {
		buf[nread] = 0;
		std::cout << "get " << nread << " bytes data \"" << buf <<
			"\" from fd " << sockfd << std::endl;
	}
	if (nread == 0 || (nread == -1 && errno != EWOULDBLOCK)) {
		reactor.remove_handler(sockfd);
		close(sockfd);
		std::cout << currtime("%T") << ": connection of fd " <<
			sockfd << " has been closed" << std::endl;
	}
}

//监听套接字的回调函数
void accept_func(int listenfd) {
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	int connfd = accept(listenfd, reinterpret_cast<struct sockaddr*>(&cliaddr), &clilen);
	std::cout << currtime("%T") << ": new connection from " <<
		sock_ntop(reinterpret_cast<const struct sockaddr*>(&cliaddr), clilen) << std::endl;
	reactor.register_handler(new Event_Handler(connfd, callback_func));	
}



int main(int argc, char* argv[])
{
	if (argc != 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	int listenfd = tcp_listen(NULL, argv[1], NULL);
	Event_Handler* acceptor = new Event_Handler(listenfd, accept_func);
	reactor.register_handler(acceptor);
	reactor.handle_events();
}
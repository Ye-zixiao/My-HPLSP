#include "MyUNP.h"
#include <iostream>
#include <unordered_map>

/**
 * 完全按照Reactor模式UML类图样式设计的服务器范例 
 */


//解多路复用器（多路分发）
class Event_Demultiplexer {
public:
    Event_Demultiplexer():epfd(epoll_create(1)){}
    ~Event_Demultiplexer() {
        close(epfd);
    }

    int register_event(struct epoll_event& event) {
        return epoll_ctl(epfd, EPOLL_CTL_ADD, event.data.fd, &event);
    }
    
    int remove_event(int fd) {
        return epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
    }

    template<int nums>
    int wait(std::array<struct epoll_event, nums>& events, int timeout) {
        return epoll_wait(epfd, events.data(), events.size(), timeout);
    }

private:
    int epfd;
};


//事件处理器，将其设置为纯虚基类纯粹是为了展示
class EventHandler {
public:
    using callback_type = void(*)(void*);

    EventHandler() = default;
    EventHandler(int fd, callback_type cb, void* da) :
        eventfd(fd), callback(cb), data(&eventfd) {}
    virtual ~EventHandler() = default;

    virtual void handler_event() = 0;
    int get_handler() const { return eventfd; }

protected:
    int eventfd;
    callback_type callback;
    void* data;
};


//I/O事件处理器
class IOEventHandler :public EventHandler {
public:
    using EventHandler::EventHandler;
    void handler_event() override { callback(data); }
};


//信号事件处理器
class SignalEventHandler :public EventHandler {
public:
    using EventHandler::EventHandler;
    void handler_event() override { callback(data); }
};


//定时事件处理器
class TimerEventHandler :public EventHandler {
public:
    using EventHandler::EventHandler;
    void handler_event() override { callback(data); }
};


//反应器
class Reactor {
public:
    
    void register_handler(EventHandler& event) {
        struct epoll_event ev;
        int eventfd = event.get_handler();

        ev.data.fd = eventfd;
        ev.events = EPOLLIN | EPOLLET;
        demultiplexer.register_event(ev);
        handlers[eventfd] = &event;
    }

    void remove_handler(int fd) {
        demultiplexer.remove_event(fd);
        delete handlers[fd];
        handlers.erase(fd);
    }

    void handler_events() {
        std::array<struct epoll_event, 10> arr;
        int nret = demultiplexer.wait<10>(arr, -1);

        for (int i = 0; i < nret; ++i) {
            handlers[arr[i].data.fd]->handler_event();
        }
    }

private:
    Event_Demultiplexer demultiplexer;
    std::unordered_map<int, EventHandler*> handlers;
};



static Reactor reactor;

void client_func(void* args) {
    int sockfd = *reinterpret_cast<int*>(args);
    char buf[MBUFSIZE];
    ssize_t nread;

    while ((nread = read(sockfd, buf, sizeof(buf) - 1)) > 0) {
        buf[nread] = 0;
        std::cout << "get " << nread << " bytes data \""
            << buf << "\" from fd " << sockfd << std::endl;
    }
    if (nread == 0 || (nread == -1 && errno != EWOULDBLOCK)) {
        reactor.remove_handler(sockfd);
        close(sockfd);
#ifdef DEBUG
        std::cout << "closed fd " << sockfd << std::endl;
#endif
    }
}

void accept_func(void* args) {
    int fd = *reinterpret_cast<int*>(args);
    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    int connfd = accept(fd, reinterpret_cast<struct sockaddr*>(&cliaddr), &clilen);
    std::cout << currtime("%T") << ": new connection from " << 
        sock_ntop(reinterpret_cast<const struct sockaddr*>(&cliaddr), clilen) << std::endl;

    IOEventHandler* clievent = new IOEventHandler(connfd, client_func, &connfd);
    reactor.register_handler(*clievent);
}


int main(int argc, char* argv[]) {
    if (argc != 2)
        err_quit("usage: %s <serv/port>", basename(argv[0]));

    int listenfd = tcp_listen(nullptr, argv[1], nullptr);
    IOEventHandler* listenhandler = new IOEventHandler(listenfd, accept_func, &listenfd);
    reactor.register_handler(*listenhandler);

    for (;;)
        reactor.handler_events();
}
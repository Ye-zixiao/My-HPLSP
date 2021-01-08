#include "MyUNP.h"
#include <event.h>
#include <iostream>

void signal_callback(int fd, short event, void* args) {
	struct event_base* base = (struct event_base*)args;
	std::cout << "\nserver closing..." << std::endl;
	event_base_loopbreak(base);
}

void client_callback(int fd, short event, void* args) {
	struct event_base* base = (struct event_base*)args;
	char buf[MBUFSIZE];
	ssize_t nread;

	while ((nread = read(fd, buf, sizeof(buf) - 1)) > 0) {
		buf[nread] = 0;
		std::cout << "get " << nread << " bytes of data \"" << buf
			<< "\" from fd " << fd << std::endl;
	}
	if (nread == 0 || (nread == -1 && errno != EWOULDBLOCK)) {
		std::cout << currtime("%T") << ": closing fd " << fd << std::endl;
		close(fd);
	}
	else {
		struct event* client_event = event_new(base, fd, EV_READ | EV_ET, client_callback, base);
		event_add(client_event, nullptr);
	}
}

void accept_callback(int fd, short event, void* args) {
	struct event_base* base = (struct event_base*)args;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(struct sockaddr_in);

	int connfd = accept(fd, (struct sockaddr*)&cliaddr, &clilen);
	std::cout << currtime("%T") << ": new connection from "
		<< sock_ntop((const struct sockaddr*)&cliaddr, clilen) << std::endl;

	struct event* client_event = event_new(base, connfd, EV_READ | EV_ET,
		client_callback, base);
	event_add(client_event, nullptr);
}


int main(int argc, char* argv[])
{
	if (argc != 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	struct event_base* base = event_base_new();

	//注册关乎SIGINT的信号处理器
	struct event* signal_event = evsignal_new(base, SIGINT, signal_callback, base);
	event_add(signal_event, nullptr);

	//注册关乎监听套接字的事件处理器
	int listenfd = tcp_listen(nullptr, argv[1], nullptr);
	struct event* listen_event = event_new(base, listenfd,
		EV_READ | EV_PERSIST | EV_ET, accept_callback, base);
	event_add(listen_event, nullptr);

	//开始事件循环
	event_base_dispatch(base);
	exit(EXIT_SUCCESS);
}
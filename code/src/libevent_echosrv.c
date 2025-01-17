#include "MyUNP.h"
#include <assert.h>
#include <event2/event.h>

/* libevent例程：从客户端读入数据，然后以ROT13算法将明文加密，
	然后返回给客户进程（没有使用边沿触发机制） */

#define MAX_LINE 16384

void do_read(evutil_socket_t fd, short events, void* arg);
void do_write(evutil_socket_t fd, short events, void* arg);

char rot13_char(char c) {
	if ((c >= 'a' && c <= 'm') || (c >= 'A' && c <= 'M'))
		return c + 13;
	else if ((c >= 'n' && c <= 'z') || (c >= 'N' && c <= 'Z'))
		return c - 13;
	else
		return c;
}

//关乎于连接套接字的相关数据
struct fd_state {
	char buffer[MAX_LINE];
	size_t buffer_used;
	size_t n_written;
	size_t write_upto;

	struct event* read_event;
	struct event* write_event;
};

struct fd_state* alloc_fd_state(struct event_base* base, evutil_socket_t fd) {
	struct fd_state* state = malloc(sizeof(struct fd_state));
	if (!state)
		return NULL;
	//创建I/O读事件处理器
	state->read_event = event_new(base, fd, EV_READ | EV_PERSIST, do_read, state);
	if (!state->read_event) {
		free(state);
		return NULL;
	}
	//创建I/O写事件处理器
	state->write_event =
		event_new(base, fd, EV_WRITE | EV_PERSIST, do_write, state);

	if (!state->write_event) {
		event_free(state->read_event);
		free(state);
		return NULL;
	}

	state->buffer_used = state->n_written = state->write_upto = 0;

	assert(state->write_event);
	return state;
}

void free_fd_state(struct fd_state* state) {
	event_free(state->read_event);
	event_free(state->write_event);
	free(state);
}

void do_read(evutil_socket_t fd, short events, void* arg) {
	struct fd_state* state = arg;
	char buf[1024];
	int i;
	ssize_t result;
	while (1) {
		assert(state->write_event);
		result = recv(fd, buf, sizeof(buf), 0);
		if (result <= 0)
			break;

		for (i = 0; i < result; ++i) {
			if (state->buffer_used < sizeof(state->buffer))
				//state->buffer[state->buffer_used++] = rot13_char(buf[i]);
				state->buffer[state->buffer_used++] = buf[i];
			if (buf[i] == '\n') {
				assert(state->write_event);
				//注册I/O写事件处理器
				event_add(state->write_event, NULL);
				state->write_upto = state->buffer_used;
			}
		}
	}

	if (result == 0) {
		close(fd);
		/* 若发生了服务器接收完所有的客户数据，但是服务器却没
			有将这些数据发送给客户怎么办？这里是有缺陷的 */
		free_fd_state(state);
	}
	else if (result < 0) {
		if (errno == EAGAIN)
			return;
		close(fd);
		perror("recv");
		free_fd_state(state);
	}
}

void do_write(evutil_socket_t fd, short events, void* arg) {
	struct fd_state* state = arg;

	while (state->n_written < state->write_upto) {
		ssize_t result = send(fd, state->buffer + state->n_written,
			state->write_upto - state->n_written, 0);
		if (result < 0) {
			if (errno == EAGAIN)
				return;
			close(fd);
			free_fd_state(state);
			return;
		}
		assert(result != 0);

		state->n_written += result;
	}

	if (state->n_written == state->buffer_used)
		state->n_written = state->write_upto = state->buffer_used = 1;

	//删除I/O读事件处理器
	event_del(state->write_event);
}

void do_accept(evutil_socket_t listener, short event, void* arg) {
	struct event_base* base = arg;
	struct sockaddr_storage ss;
	socklen_t slen = sizeof(ss);
	int fd = accept(listener, (struct sockaddr*)&ss, &slen);
	if (fd < 0) { // XXXX eagain??
		perror("accept");
	}
	else if (fd > FD_SETSIZE) {
		close(fd);
	}
	else {
		struct fd_state* state;
		evutil_make_socket_nonblocking(fd);
		state = alloc_fd_state(base, fd);
		assert(state);
		assert(state->write_event);
		//注册I/O读事件处理器
		event_add(state->read_event, NULL);
	}
}

void run(void) {
	evutil_socket_t listener;
	struct sockaddr_in sin;
	struct event_base* base;
	struct event* listener_event;

	//创建Reactor反应器，它会执行事件循环
	base = event_base_new();
	if (!base)
		return;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(12000);

	listener = socket(AF_INET, SOCK_STREAM, 0);
	evutil_make_socket_nonblocking(listener);

#ifndef WIN32
	{
		int one = 1;
		setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	}
#endif

	if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		perror("bind");
		return;
	}

	if (listen(listener, 16) < 0) {
		perror("listen");
		return;
	}

	//创建监听套接字的事件处理器
	listener_event = event_new(base, listener, EV_READ | EV_PERSIST, do_accept, (void*)base);
	event_add(listener_event, NULL);

	event_base_dispatch(base);
}

int main(int argc, char** argv) {
	setvbuf(stdout, NULL, _IONBF, 0);

	run();
	return 0;
}

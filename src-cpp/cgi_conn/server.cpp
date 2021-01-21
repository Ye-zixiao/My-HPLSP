#include "ProcessPool.h"

/* 读取用户的数据，并根据用户发送过来的数据执行指定的程序 */
class CGI_Conn {
public:
	static constexpr int BUFFER_SIZE = 1024;

	CGI_Conn() = default;
	~CGI_Conn() = default;

	void init(int epfd, int sockfd, const struct sockaddr_in& cliaddr);
	void process();

private:
	static int m_epfd;

private:
	struct sockaddr_in m_cliaddr;
	char m_buf[BUFFER_SIZE];
	int m_sockfd;
	int m_readidx;
};

int CGI_Conn::m_epfd = -1;

void 
CGI_Conn::init(int epfd, int sockfd, const struct sockaddr_in& cliaddr) {
	//bzero(m_buf, sizeof(m_buf));
	m_cliaddr = cliaddr;
	m_sockfd = sockfd;
	m_epfd = epfd;
	m_readidx = 0;
}


void CGI_Conn::process() {
	int idx = m_readidx;
	ssize_t nread;
	pid_t pid;

	while ((nread = read(m_sockfd, m_buf + m_readidx, sizeof(m_buf) - 1 - m_readidx)) > 0) {
		m_readidx += nread;
		for (; idx < m_readidx; ++idx)
			if (m_buf[idx] >= 1 && m_buf[idx - 1] == '\r' && m_buf[idx] == '\n')
				break;
		if (idx != m_readidx) {
			m_buf[idx - 1] = 0;
			break;
		}
	}
	if (nread == 0 || (nread == -1 && (errno != EWOULDBLOCK || errno != EINTR))) {
		rmfdepoll(m_epfd, m_sockfd);
		return;
	}
	else if (nread == -1 && (errno == EWOULDBLOCK || errno == EINTR))
		return;

	if (access(m_buf, X_OK) == -1)
		rmfdepoll(m_epfd, m_sockfd);
	else {
		if ((pid = fork()) == -1 || pid > 0)
			rmfdepoll(m_epfd, m_sockfd);
		else if (pid == 0) {
			dup2(m_sockfd, STDIN_FILENO);
			dup2(m_sockfd, STDOUT_FILENO);
			execl(m_buf, m_buf, (char*)NULL);
			err_sys("execl error");
		}
	}
}

int main(int argc, char* argv[])
{
	if (argc < 2)
		err_quit("usage: %s <serv/port>", basename(argv[0]));

	int listenfd = tcp_listen(nullptr, argv[1], nullptr);
	Process_Pool<CGI_Conn>* pool = Process_Pool<CGI_Conn>::create(listenfd);
	if (pool) { pool->run(); delete pool; }
	close(listenfd);
	exit(EXIT_SUCCESS);
}

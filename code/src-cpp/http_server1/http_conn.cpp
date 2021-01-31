#include "http_conn.h"
#include <sys/mman.h>
#include <iostream>
#include <stdarg.h>

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax "
						"or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission "
						"to get file form this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not "
							"found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem "
							"serving the requested file.\n";
const char* doc_root = "/usr/share/nginx/html";

int http_conn::m_user_cnt = 0;
int http_conn::m_epfd = -1;


void addfd(int epfd, int fd, bool one_shot) {
	struct epoll_event event;
	event.data.fd = fd;
	event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	if (one_shot) event.events |= EPOLLONESHOT;
	epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &event);
	setnonblocking(fd);
}

void removefd(int epfd, int fd) {
	epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
	close(fd);
}

void modfd(int epfd, int fd, int ev) {
	struct epoll_event event;
	event.data.fd = fd;
	event.events = ev | EPOLLET | EPOLLONESHOT | EPOLLRDHUP;
	epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &event);
}

void http_conn::close_conn(bool real_close) {
	if (real_close && m_sockfd != -1) {
		removefd(m_epfd, m_sockfd);
		m_sockfd = -1;
		m_user_cnt--;
	}
}

void http_conn::init(int sockfd, const struct sockaddr_in& addr) {
	const int on = 1;
	m_sockfd = sockfd;
	m_addr = addr;
	setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	addfd(m_epfd, sockfd, true);
	m_user_cnt++;
	
	init();
}

/* 对除客户信息之外的内部数据进行初始化 */
void http_conn::init() {
	m_check_state = CHECK_STATE_REQUESTLINE;
	m_linger = false;

	m_method = GET;
	m_version = nullptr;
	m_url = nullptr;
	m_host = nullptr;
	m_content_len = 0;
	m_start_line = 0;
	m_check_idx = 0;
	m_read_idx = 0;
	m_write_idx = 0;
	bzero(m_read_buf, READ_BUFFER_SIZE);
	bzero(m_write_buf, WRITE_BUFFER_SIZE);
	bzero(m_real_file, FILENAME_LEN);
}

/* 从内部缓冲区中提取出一个HTTP行，并返回提取读取状态 */
http_conn::LINE_STATUS http_conn::parse_line() {
	char ch;
	for (; m_check_idx < m_read_idx; ++m_check_idx) {
		ch = m_read_buf[m_check_idx];
		if (ch == '\r') {
			if ((m_check_idx + 1) == m_read_idx)
				return LINE_OPEN;
			else if (m_read_buf[m_check_idx + 1] == '\n') {
				m_read_buf[m_check_idx++] = '\0';
				m_read_buf[m_check_idx++] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
		else if (ch == '\n') {
			if (m_check_idx > 1 && m_read_buf[m_check_idx - 1] == '\r') {
				m_read_buf[m_check_idx - 1] = '\0';
				m_read_buf[m_check_idx] = '\0';
				return LINE_OK;
			}
			return LINE_BAD;
		}
	}
	return LINE_OPEN;
}

bool http_conn::read() {
	/* 也就意味着这个缓冲区在请求的过程中只使用了一次 */
	if (m_read_idx >= READ_BUFFER_SIZE)
		return false;

	ssize_t nread;
	while ((nread = ::read(m_sockfd, m_read_buf + m_read_idx,
		READ_BUFFER_SIZE - m_read_idx)) > 0)
		m_read_idx += nread;
	if (nread == 0 || (nread == -1 && errno != EWOULDBLOCK))
		return false;
	return true;
}

/* 解析HTTP请求行，例如：GET /index.html HTTP/1.1\r\n */
http_conn::HTTP_CODE http_conn::parse_request_line(char* text) {
	if (!(m_url = strpbrk(text, " \t")))
		return BAD_REQUEST;
	*m_url++ = '\0';

	//获得HTTP请求行中的方法字段
	char* method = text;
	if (strcasecmp(method, "GET") == 0)
		m_method = GET;
	else return BAD_REQUEST;

	//获得HTTP请求行中的URL字段和version字段
	m_url += strspn(m_url, " \t");
	m_version = strpbrk(m_url, " \t");
	if (!m_version) return BAD_REQUEST;
	*m_version++ = '\0';
	m_version += strspn(m_version, " \t");

	if (strcasecmp(m_version, "HTTP/1.1") != 0)
		return BAD_REQUEST;
	if (strncasecmp(m_url, "http://", 7) == 0) {
		m_url += 7;
		m_url = strchr(m_url, '/');
	}
	if (!m_url || m_url[0] != '/')
		return BAD_REQUEST;

	//设置主状态机状态，并返回http解析结果
	m_check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
}

/* 解析HTTP普通头部行，支持Connection、Content-Length、Host字段 */
http_conn::HTTP_CODE http_conn::parse_headers(char* text) {
	if (text[0] == '\0') {
		if (m_content_len != 0) {
			m_check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		return GET_REQUEST;
	}
	else if (strncasecmp(text, "Connection:", 11) == 0) {
		text += 11;
		text += strspn(text, " \t");
		if (strcasecmp(text, "keep-alive") == 0)
			m_linger = true;
	}
	else if (strncasecmp(text, "Content-Length:", 15) == 0) {
		text += 15;
		text += strspn(text, " \t");
		m_content_len = atol(text);
	}
	else if (strncasecmp(text, "Host:", 5) == 0) {
		text += 5;
		text += strspn(text, " \t");
		m_host = text;
	}
	else std::cout << "oops! not support \"" << text << "\"" << std::endl;
	
	return NO_REQUEST;
}

/* 解析HTTP内容部分 */
http_conn::HTTP_CODE http_conn::parse_content(char* text) {
	if (m_read_idx >= (m_content_len + m_check_idx)) {
		text[m_content_len] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

/* 处理读取之后在缓冲区中的内容，并对其进行http解析、处理（注意：读取
	和HTTP解析这两个过程是分离的，读取是由主线程完成，而HTTP解析是由
	工作线程完成的） */
http_conn::HTTP_CODE http_conn::process_read() {
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;
	char* text = nullptr;

	while ((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) ||
		(line_status = parse_line()) == LINE_OK) {
		text = get_line();
		m_start_line = m_check_idx;
		std::cout << "got 1 http line: " << text << std::endl;

		//根据主状态机状态对不同的行执行不同的解析操作
		switch (m_check_state) {
		case CHECK_STATE_REQUESTLINE:
			if ((ret = parse_request_line(text)) == BAD_REQUEST)
				return BAD_REQUEST;
			break;
		case CHECK_STATE_HEADER:
			if ((ret = parse_headers(text)) == BAD_REQUEST)
				return BAD_REQUEST;
			else if (ret == GET_REQUEST)
				return do_request();
			break;
		case CHECK_STATE_CONTENT:
			if ((ret = parse_content(text)) == GET_REQUEST)
				return do_request();
			line_status = LINE_OPEN;
			break;
		default:
			return INTERNAL_ERROR;
		}
	}
	return NO_REQUEST;
}

/* 根据GET请求字段获取的URL字段打开指定文件，将其写入到存储映射区中 */
http_conn::HTTP_CODE http_conn::do_request() {
	strcpy(m_real_file, doc_root);
	int len = strlen(doc_root);
	strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
	if (strcmp(m_real_file + len, "/") == 0)
		strcpy(m_real_file + len + 1, "index.html");

	if (stat(m_real_file, &m_file_stat) == -1)
		return NO_RESOURCE;
	if (!(m_file_stat.st_mode & S_IROTH))
		return FORBIDDEN_REQUEST;
	if (S_ISDIR(m_file_stat.st_mode))
		return BAD_REQUEST;
	int fd = open(m_real_file, O_RDONLY);
	m_file_addr = (char*)mmap(nullptr, m_file_stat.st_size, PROT_READ,
		MAP_PRIVATE, fd, 0);
	close(fd);
	return FILE_REQUEST;
}

void http_conn::unmap() {
	if (m_file_addr) {
		munmap(m_file_addr, m_file_stat.st_size);
		m_file_addr = nullptr;
	}
}

bool http_conn::write() {
	ssize_t nwrite;
	int bytes_have_send = 0;
	int bytes_to_send = m_write_idx;
	//若没有东西要写，则重新关注于连接套接字的读事件
	if (bytes_to_send == 0) {
		modfd(m_epfd, m_sockfd, EPOLLIN);
		init();
		return true;
	}

	while (bytes_have_send < bytes_to_send) {
		if ((nwrite = writev(m_sockfd, m_iov, m_iov_cnt)) == -1) {
			if (errno == EWOULDBLOCK) {
				modfd(m_epfd, m_sockfd, EPOLLOUT);
				return true;
			}
			unmap();
			return false;
		}
		bytes_have_send += nwrite;
	}

	unmap();
	if (m_linger) {
		init();
		modfd(m_epfd, m_sockfd, EPOLLIN);
		return true;
	}
	return false;
}

/* 将指定的内容写入到写缓冲区。一般用来创建回复给客户时的HTTP报文 */
bool http_conn::add_response(const char* fmt, ...) {
	if (m_write_idx >= WRITE_BUFFER_SIZE)
		return false;

	va_list arg_list;
	va_start(arg_list, fmt);
	int len = vsnprintf(m_write_buf + m_write_idx, WRITE_BUFFER_SIZE
		- 1 - m_write_idx, fmt, arg_list);
	if (len >= (WRITE_BUFFER_SIZE - 1 - m_write_idx))
		return false;
	m_write_idx += len;
	va_end(arg_list);
	return true;
}

bool http_conn::add_status_line(int status, const char* title) {
	return add_response("%s %d %s\r\n", "HTTP/1.1", status, title);
}

bool http_conn::add_headers(int content_len) {
	bool ret = true;
	ret = add_content_length(content_len);
	ret = add_linger();
	ret = add_blank_line();
	return ret;
}

bool http_conn::add_content_length(int content_len) {
	return add_response("Content-Length: %d\r\n", content_len);
}

bool http_conn::add_linger() {
	return add_response("Connection: %s\r\n", (m_linger ? "keep-alive" : "close"));
}

bool http_conn::add_blank_line() {
	return add_response("%s","\r\n");
}

bool http_conn::add_content(const char* content) {
	return add_response("%s", content);
}

/* 根据process_read()的解析处理结果，创建HTTP回复报文，然后发送给客户 */
bool http_conn::process_write(HTTP_CODE ret) {
	switch (ret) {
	case INTERNAL_ERROR:
		add_status_line(500, error_500_title);
		add_headers(strlen(error_500_form));
		if (!add_content(error_500_form)) return false;
		break;
	case BAD_REQUEST:
		add_status_line(400, error_400_title);
		add_headers(strlen(error_400_form));
		if (!add_content(error_400_form)) return false;
		break;
	case FORBIDDEN_REQUEST:
		add_status_line(403, error_403_title);
		add_headers(strlen(error_403_form));
		if (!add_content(error_403_form)) return false;
		break;
	case FILE_REQUEST:
		add_status_line(200, ok_200_title);
		if (m_file_stat.st_size) {
			add_headers(m_file_stat.st_size);
			//使m_iov[0]指向HTTP回复报文的头部字段
			m_iov[0].iov_base = m_write_buf;
			m_iov[0].iov_len = m_write_idx;
			//使m_iov[1]指向文件数据
			m_iov[1].iov_base = m_file_addr;
			m_iov[1].iov_len = m_file_stat.st_size;
			m_iov_cnt = 2;
			return true;
		}
		else {
			const char* ok_string = "<html><body></body></html>";
			add_headers(strlen(ok_string));
			if (!add_content(ok_string)) return false;
		}
		break;
	default:
		return false;
	}

	m_iov[0].iov_base = m_write_buf;
	m_iov[0].iov_len = m_write_idx;
	m_iov_cnt = 1;
	return true;
}

/* 工作线程处理HTTP请求任务的入口函数 */
void http_conn::process() {
	HTTP_CODE read_ret = process_read();
	if (read_ret == NO_REQUEST) {
		modfd(m_epfd, m_sockfd, EPOLLIN);
		return;
	}
	//if (!process_write(read_ret))
	//	close_conn();
	///* 处理完毕后因为相关的HTTP回复报文数据已经写入到缓冲区，
	//	所以向内核epoll事件表注册写事件 */
	//modfd(m_epfd, m_sockfd, EPOLLOUT);
	if (process_write(read_ret))
		modfd(m_epfd, m_sockfd, EPOLLOUT);
	else close_conn();
}
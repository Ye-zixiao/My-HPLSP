#include "http_conn.h"
#include <sys/mman.h>
#include <iostream>
#include <stdarg.h>

const char* ok_200_title = "OK";
const char* error_400_title = "Bad Request";
const char* error_400_form = "Your request has bad syntax \
				or is inherently impossible to satisfy.\n";
const char* error_403_title = "Forbidden";
const char* error_403_form = "You do not have permission \
						to get file form this server.\n";
const char* error_404_title = "Not Found";
const char* error_404_form = "The requested file was not \
							found on this server.\n";
const char* error_500_title = "Internal Error";
const char* error_500_form = "There was an unusual problem \
							serving the requested file.\n";
const char* doc_root = "/usr/share/nginx/html";


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

int http_conn::m_user_cnt = 0;
int http_conn::m_epfd = -1;


void http_conn::close_conn(bool real_close) {
	if (real_close && m_sockfd != -1) {
		removefd(m_epfd, m_sockfd);
		m_sockfd = -1;
		m_user_cnt--;
	}
}

void http_conn::init(int sockfd, const struct sockaddr_in& addr) {
	m_sockfd = sockfd;
	m_addr = addr;
	int reuse = 1;
	setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
	addfd(m_epfd, sockfd, true);
	m_user_cnt++;
	
	init();
}

void http_conn::init() {
	m_check_state = CHECK_STATE_REQUESTLINE;
	m_linger = false;

	m_method = GET;
	m_url = nullptr;
	m_version = nullptr;
	m_content_len = 0;
	m_host = 0;
	m_start_line = 0;
	m_check_idx = 0;
	m_read_idx = 0;
	m_write_idx = 0;
	bzero(m_read_buf, READ_BUFFER_SIZE);
	bzero(m_write_buf, WRITE_BUFFER_SIZE);
	bzero(m_real_file, FILENAME_LEN);
}

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

http_conn::HTTP_CODE http_conn::parse_request_line(char* text) {
	if (!(m_url = strpbrk(text, " \t")))
		return BAD_REQUEST;
	*m_url++ = '\0';

	char* method = text;
	if (strcasecmp(method, "GET") == 0)
		m_method = GET;
	else return BAD_REQUEST;

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
	m_check_state = CHECK_STATE_HEADER;
	return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_headers(char* text) {
	if (text[0] == '\0') {
		if (m_content_len != 0) {
			m_check_state = CHECK_STATE_CONTENT;
			return NO_REQUEST;
		}
		std::cerr<<"get"<<std::endl;
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
	else std::cout << "oop! unknow header " << text << std::endl;
	
	return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::parse_content(char* text) {
	if (m_read_idx >= (m_content_len + m_check_idx)) {
		text[m_content_len] = '\0';
		return GET_REQUEST;
	}
	return NO_REQUEST;
}

http_conn::HTTP_CODE http_conn::process_read() {
	LINE_STATUS line_status = LINE_OK;
	HTTP_CODE ret = NO_REQUEST;
	char* text = nullptr;

	while ((m_check_state == CHECK_STATE_CONTENT && line_status == LINE_OK) ||
		(line_status = parse_line()) == LINE_OK) {
		text = get_line();
		m_start_line = m_check_idx;
		std::cout << "got 1 http line: " << text << "|" << std::endl;

		switch (m_check_state) {
		case CHECK_STATE_REQUESTLINE:
			std::cerr<<"1"<<std::endl;
			if ((ret = parse_request_line(text)) == BAD_REQUEST)
				return BAD_REQUEST;
			break;
		case CHECK_STATE_HEADER:
			std::cerr<<"2"<<std::endl;
			if ((ret = parse_headers(text)) == BAD_REQUEST)
				return BAD_REQUEST;
			else if (ret == GET_REQUEST){
				debug();
				return do_request();		
			}
			break;
		case CHECK_STATE_CONTENT:
			std::cerr<<"3"<<std::endl;
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

http_conn::HTTP_CODE http_conn::do_request() {
	strcpy(m_real_file, doc_root);
	int len = strlen(doc_root);
	strncpy(m_real_file + len, m_url, FILENAME_LEN - len - 1);
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
	debug();
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

		bytes_to_send -= nwrite;
		bytes_have_send += nwrite;
	}

	unmap();
	if (m_linger) {
		init();
		modfd(m_epfd, m_sockfd, EPOLLIN);
		return true;
	}
	modfd(m_epfd, m_sockfd, EPOLLIN);
	return false;
}

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
	add_content_length(content_len);
	add_linger();
	add_blank_line();
	return true;
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
		std::cerr<<"p1"<<std::endl;
		add_status_line(200, ok_200_title);
		std::cerr<<"p2"<<std::endl;
		if (m_file_stat.st_size) {
			add_headers(m_file_stat.st_size);
			//构建http应答报文中的头部信息
			m_iov[0].iov_base = m_write_buf;
			m_iov[0].iov_len = m_write_idx;
			//填充文件数据
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
	m_iov_cnt=1;
	return true;
}

void http_conn::process() {
	HTTP_CODE read_ret = process_read();
	if (read_ret == NO_REQUEST) {
		modfd(m_epfd, m_sockfd, EPOLLIN);
		return;
	}
	if(read_ret==FILE_REQUEST)
		debug();
	debug();
	if (!process_write(read_ret)){
		std::cerr<<"fu"<<std::endl;
		close_conn();		
	}
	debug();
	modfd(m_epfd, m_sockfd, EPOLLOUT);
	debug();
}

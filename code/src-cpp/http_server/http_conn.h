#ifndef HTTPCONNECTION_H_
#define HTTPCONNECTION_H_

#include "MyUNP.h"
#include "synchronize.h"

/* http请求任务对象。要求工作线程解析http请求，并向客户
	返回指定的数据资源或者告知相关的错误	*/
class http_conn {
public:
	static constexpr int FILENAME_LEN = 200;
	static constexpr int READ_BUFFER_SIZE = 2048;
	static constexpr int WRITE_BUFFER_SIZE = 1024;
	enum METHOD {
		GET, POST, HEAD, PUT, DELETE,
		TRACE, OPTIONS, CONNECT, PATCH
	};
	enum CHECK_STATE {	//主状态机状态，表示下一步或者当前正在处理（但未处理完）xx部分
		CHECK_STATE_REQUESTLINE,
		CHECK_STATE_HEADER, CHECK_STATE_CONTENT
	};
	enum HTTP_CODE {	//http解析结果
		NO_REQUEST, GET_REQUEST, BAD_REQUEST,
		NO_RESOURCE, FORBIDDEN_REQUEST, FILE_REQUEST,
		INTERNAL_ERROR, CLOSED_CONNECTION
	};
	enum LINE_STATUS {	//行读取状态
		LINE_OK, LINE_BAD, LINE_OPEN
	};

public:
	http_conn() = default;
	~http_conn() = default;

public:
	void init(int sockfd, const struct sockaddr_in& addr);
	void close_conn(bool real_close = true);
	void process();
	bool read();
	bool write();

private:
	void init();
	HTTP_CODE process_read();
	bool process_write(HTTP_CODE ret);
	HTTP_CODE parse_request_line(char* text);
	HTTP_CODE parse_headers(char* text);
	HTTP_CODE parse_content(char* text);
	HTTP_CODE do_request();
	char* get_line() { return m_read_buf + m_start_line; }
	LINE_STATUS parse_line();

	void unmap();
	bool add_response(const char* format, ...);
	bool add_content(const char* content);
	bool add_content_length(int content_len);
	bool add_status_line(int status, const char* title);
	bool add_headers(int content_length);
	bool add_linger();
	bool add_blank_line();

public:
	static int m_epfd;
	//记录着整个服务器程序所服务的客户数量，可能有一点歧义
	static int m_user_cnt;

private:
	struct sockaddr_in m_addr;
	int m_sockfd;
	bool m_linger;

	char m_read_buf[READ_BUFFER_SIZE];
	int m_start_line;
	int m_check_idx;
	int m_read_idx;
	
	char m_write_buf[WRITE_BUFFER_SIZE];
	int m_write_idx;
	
	//当前解析状态
	CHECK_STATE m_check_state;
	//当前所请求的方法
	METHOD m_method;

	//http请求的解析结果和向客户回复时需要使用的信息
	char m_real_file[FILENAME_LEN];
	char* m_url;
	char* m_version;
	char* m_host;
	int m_content_len;

	char* m_file_addr;
	struct stat m_file_stat;
	struct iovec m_iov[2];
	int m_iov_cnt;
};


#endif // !HTTPCONNECTION_H_

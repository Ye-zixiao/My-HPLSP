#ifndef HTTP_READXF3_H_
#define HTTP_READXF3_H_


/* 主状态机状态 */
enum CHECK_STATUS {
    CHECK_STATE_REQUESTLINE, //此时应该调用分析请求行函数
    CHECK_STATE_HEADER       //此时应该调用分析首部行函数
};


/* 从状态机状态，反应对HTTP报文中行的读取状态*/
enum LINE_STATUS { LINE_OK, LINE_BAD, LINE_OPEN };


/* 服务器处理HTTP请求结果 */
enum HTTP_CODE {
    NO_REQUEST,         //请求不完整，需要继续读取数据
    GET_REQUEST,        //获取到完整的GET请求
    BAD_REQUEST,        //客户请求语法有问题
    FORBINDDEN_REQUEST, //客户对资源没有访问权限
    INTERNAL_ERROR,     //服务器内部错误
    CLOSED_CONNECTION   //连接已关闭
};


extern const char* szret[];

int parse_content(char* buffer, int* check_index, int* checkstate,
        int* read_index, int* start_line);
int parse_line(char* buffer, int* check_index, int* read_index);
int parse_requestline(char* temp, int* checkstate);
int parse_headers(char* temp);



#endif // !HTTP_READXF3_H_
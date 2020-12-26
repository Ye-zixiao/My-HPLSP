#include "http_read.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>

const char* szret[] = {
    "I get a correct result\n",
    "Something wrong\n"
};


/* 从状态机解析出一行的内容，并在告知调用者当前缓冲区中
 * 是否存在一个完整的HTTP行
 * @param buffer		缓冲区指针
 * @param chk_idx	指向当前检查字符下标
 * @param read_idx	指向当前缓冲区的边界
 * @return			缓冲区行状态
 */
int parse_line(char* buffer, int* check_idx, int* read_idx) {
    char temp;

    for (; *check_idx < *read_idx; ++*check_idx) {
        temp = buffer[*check_idx];
        if (temp == '\r') {
            if (*check_idx + 1 == *read_idx)
                return LINE_OPEN;
            //若当前缓冲区中的数据存在\r\n，则将这部分的"\r\n"转换成"\0\0"
            else if (buffer[(*check_idx) + 1] == '\n') {
                buffer[(*check_idx)++] = '\0';
                buffer[(*check_idx)++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n') {
            if (*check_idx > 1 && buffer[*check_idx + 1] == '\r') {
                buffer[*check_idx - 1] = '\0';
                buffer[(*check_idx)++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}


/* 解析HTTP报文中的请求行。在处理的过程中还会在相应的位置上设置'\0'字符 */
int parse_requestline(char* temp, int* chkstate) {
    char* url, * method, * version;

    if ((url = strpbrk(temp, " \t")) == NULL)
        return BAD_REQUEST;
    *url++ = '\0';

    //使method指针定位到请求行的方法字符串
    method = temp;
    if (strcasecmp(method, "GET") == 0)
        printf("The request is GET\n");
    else return BAD_REQUEST;
    
    //使version字段定位到请求行的HTTP版本字符串
    url += strspn(url, " \t");
    if ((version = strpbrk(url, " \t")) == NULL)
        return BAD_REQUEST;
    *version++ = '\0';
    if (strcasecmp(version, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    
    //url定位到请求文件路径字符串，提取出路径文件路径
    if (strncasecmp(url, "http://", 7) == 0) {
        url += 7;
        url = strchr(url, '/');
    }
    if (!url || url[0] != '/')
        return BAD_REQUEST;

    printf("The request URL is: %s\n", url);
    *chkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}


/* 处理HTTP报文段中的首部字段，仅支持Host字段 */
int parse_headers(char* temp) {
    if (temp[0] == '\0')
        return GET_REQUEST;
    else if (strncasecmp(temp, "Host:", 5) == 0) {
        temp += 5;
        temp += strspn(temp, " \t");
        printf("The request host is: %s\n", temp);
    }
    else
        printf("I can't handle this header\n");
    return NO_REQUEST;
}


/* 分析缓冲区中的内容 */
int parse_content(char* buffer, int* chkidx, int* chkstate,
    int* readidx, int* start_line) {
    //主状态机的状态是全局的，从状态机对行的分析是局部的
    int linestatus = LINE_OK;
    int retcode = NO_REQUEST;
    char* temp;

    while ((linestatus = parse_line(buffer, chkidx, readidx)) == LINE_OK) {
        temp = buffer + *start_line;
        *start_line = *chkidx;

        switch (*chkstate) {
        case CHECK_STATE_REQUESTLINE:
            //分析HTTP报文请求行
            if (parse_requestline(temp, chkstate) == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        case CHECK_STATE_HEADER:
            //分析HTTP报文首部行
            if ((retcode = parse_headers(temp)) == BAD_REQUEST)
                return BAD_REQUEST;
            else if (retcode == GET_REQUEST)
                return GET_REQUEST;
            break;
        default:
            return INTERNAL_ERROR;
        }
    }

    if (linestatus == LINE_OPEN)
        return NO_REQUEST;
    return BAD_REQUEST;
}
#ifndef MYERROR_2FDXFS9_H_
#define MYERROR_2FDXFS9_H_

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


#define ERRMSG_LEN	128

extern int daemon_proc;

void err_ret(const char* fmt, ...);
void err_cont(int error, const char* fmt, ...);
void err_sys(const char* fmt, ...);
void err_exit(int error, const char* fmt, ...);
void err_dump(const char* fmt, ...);
void err_msg(const char* fmt, ...);
void err_quit(const char* fmt, ...);

void debug(void);


#ifdef __cplusplus
}
#endif // __cplusplus

#endif // !MYERROR2FDXFS9_H_

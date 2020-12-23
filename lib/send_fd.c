#include "MyUNP.h"

union Cmsg {
	struct cmsghdr cmsg;
	char control[CMSG_SPACE(sizeof(int))];
};


/* 向指定UNIX域套接字发送一个文件描述符 */
ssize_t send_fd(int fd, void* vptr, size_t nbytes, int sendfd) {
	union Cmsg control_un;
	struct cmsghdr* cmptr;
	struct msghdr msg;
	struct iovec iov[1];

	iov[0].iov_base = vptr;
	iov[0].iov_len = nbytes;
	bzero(&msg, sizeof(msg));
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);
	
	cmptr = CMSG_FIRSTHDR(&msg);
	cmptr->cmsg_len = CMSG_LEN(sizeof(int));
	cmptr->cmsg_level = SOL_SOCKET;
	cmptr->cmsg_type = SCM_RIGHTS;
	*((int*)CMSG_DATA(cmptr)) = sendfd;

	return sendmsg(fd, &msg, 0);
}
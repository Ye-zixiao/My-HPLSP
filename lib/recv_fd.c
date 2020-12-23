#include "MyUNP.h"

//这种通过联合来实现指定足量空间分配也是值得学习的
union Cmsg {
	struct cmsghdr cmsg;
	char control[CMSG_SPACE(sizeof(int))];
};


/* 从指定的UNIX域套接字中接收一个文件描述符和数据 */
ssize_t recv_fd(int sockfd, void* vptr, size_t nbytes, int* recvfd) {
	union Cmsg control_un;
	struct cmsghdr* cmptr;
	struct msghdr msg;
	struct iovec iov[1];
	ssize_t nread;

	bzero(&msg, sizeof(msg));
	iov[0].iov_base = vptr;
	iov[0].iov_len = nbytes;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	msg.msg_control = control_un.control;
	msg.msg_controllen = sizeof(control_un.control);

	if ((nread = recvmsg(sockfd, &msg, 0)) <= 0)
		return nread;

	if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
		cmptr->cmsg_len == CMSG_LEN(sizeof(int))) {
		if (cmptr->cmsg_level != SOL_SOCKET)
			err_quit("control message level != SOL_SOCKET");
		if (cmptr->cmsg_type != SCM_RIGHTS)
			err_quit("control message type != SCM_RIGHTS");
		*recvfd = *((int*)CMSG_DATA(cmptr));
	}
	else *recvfd = -1;
	return nread;
}
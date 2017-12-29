int read_fd(int fd, void *ptr, int nbytes, int *recvfd)
{
    struct msghdr msg;
    struct iovec iov[1];
    int n;
    int newfd;
#ifdef HAVE_MSGHDR_MSG_CONTROL
    union{ // 对齐
	struct cmsghdr cm;
	char control[CMSG_SPACE(sizeof(int))];
    }control_un;
    struct cmsghdr *cmptr;
    // 设置辅助数据缓冲区和长度
    msg.msg_control = control_un.control;
    msg.msg_controllen = sizeof(control_un.control);
#else
    msg.msg_accrights = (caddr_t) &newfd; // 这个简单
    msg.msg_accrightslen = sizeof(int);
#endif 
    
    // TCP无视
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    // 设置数据缓冲区
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    // 设置结束，准备接收
    if((n = recvmsg(fd, &msg, 0)) <= 0)
    {
        return n;
    }
#ifdef HAVE_MSGHDR_MSG_CONTROL
    // 检查是否收到了辅助数据，以及长度，回忆上一节的CMSG宏
    cmptr = CMSG_FIRSTHDR(&msg);
    if((cmptr != NULL) && (cmptr->cmsg_len == CMSG_LEN(sizeof(int))))
    {
	// 还是必要的检查
        if(cmptr->cmsg_level != SOL_SOCKET)
        {
            printf("control level != SOL_SOCKET/n");
            exit(-1);
        }
        if(cmptr->cmsg_type != SCM_RIGHTS)
        {
            printf("control type != SCM_RIGHTS/n");
            exit(-1);
        }
	// 好了，描述符在这
        *recvfd = *((int*)CMSG_DATA(cmptr));
    }
    else
    {
        if(cmptr == NULL) printf("null cmptr, fd not passed./n");
        else printf("message len[%d] if incorrect./n", cmptr->cmsg_len);
        *recvfd = -1; // descriptor was not passed
    }
#else
    if(msg.msg_accrightslen == sizeof(int)) *recvfd = newfd; 
    else *recvfd = -1;
#endif
    return n;
}

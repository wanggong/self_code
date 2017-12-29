int write_fd(int fd, void *ptr, int nbytes, int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];
    // 有些系统使用的是旧的msg_accrights域来传递描述符，Linux下是新的msg_control字段
#ifdef HAVE_MSGHDR_MSG_CONTROL
    union{ // 前面说过，保证cmsghdr和msg_control的对齐
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    }control_un;
    struct cmsghdr *cmptr; 
    // 设置辅助缓冲区和长度
    msg.msg_control = control_un.control; 
    msg.msg_controllen = sizeof(control_un.control);
    // 只需要一组附属数据就够了，直接通过CMSG_FIRSTHDR取得
    cmptr = CMSG_FIRSTHDR(&msg);
    // 设置必要的字段，数据和长度
    cmptr->cmsg_len = CMSG_LEN(sizeof(int)); // fd类型是int，设置长度
    cmptr->cmsg_level = SOL_SOCKET; 
    cmptr->cmsg_type = SCM_RIGHTS;  // 指明发送的是描述符
    *((int*)CMSG_DATA(cmptr)) = sendfd; // 把fd写入辅助数据中
#else
    msg.msg_accrights = (caddr_t)&sendfd; // 这个旧的更方便啊
    msg.msg_accrightslen = sizeof(int);
#endif
    // UDP才需要，无视
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    // 别忘了设置数据缓冲区，实际上1个字节就够了
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    return sendmsg(fd, &msg, 0);
}
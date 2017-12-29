int write_fd(int fd, void *ptr, int nbytes, int sendfd)
{
    struct msghdr msg;
    struct iovec iov[1];
    // ��Щϵͳʹ�õ��Ǿɵ�msg_accrights����������������Linux�����µ�msg_control�ֶ�
#ifdef HAVE_MSGHDR_MSG_CONTROL
    union{ // ǰ��˵������֤cmsghdr��msg_control�Ķ���
        struct cmsghdr cm;
        char control[CMSG_SPACE(sizeof(int))];
    }control_un;
    struct cmsghdr *cmptr; 
    // ���ø����������ͳ���
    msg.msg_control = control_un.control; 
    msg.msg_controllen = sizeof(control_un.control);
    // ֻ��Ҫһ�鸽�����ݾ͹��ˣ�ֱ��ͨ��CMSG_FIRSTHDRȡ��
    cmptr = CMSG_FIRSTHDR(&msg);
    // ���ñ�Ҫ���ֶΣ����ݺͳ���
    cmptr->cmsg_len = CMSG_LEN(sizeof(int)); // fd������int�����ó���
    cmptr->cmsg_level = SOL_SOCKET; 
    cmptr->cmsg_type = SCM_RIGHTS;  // ָ�����͵���������
    *((int*)CMSG_DATA(cmptr)) = sendfd; // ��fdд�븨��������
#else
    msg.msg_accrights = (caddr_t)&sendfd; // ����ɵĸ����㰡
    msg.msg_accrightslen = sizeof(int);
#endif
    // UDP����Ҫ������
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    // �������������ݻ�������ʵ����1���ֽھ͹���
    iov[0].iov_base = ptr;
    iov[0].iov_len = nbytes;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    return sendmsg(fd, &msg, 0);
}
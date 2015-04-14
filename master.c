#include "master.h"

master_t master_create()
{
    master_t pmaster = (master_t)malloc(sizeof(master));

    if (pmaster == NULL)
    {
        printf("malloc master error\n");
        return NULL;
    }

    pmaster->tid = pthread_self();
    pmaster->epfd = -1;;
    pmaster->listenfd = -1;
    pmaster->accept_count = 0;

    pmaster->epfd = epoll_create(256);

    return pmaster;
}

void master_close(master_t pmaster)
{
    if (pmaster == NULL)
        return;

    //必须先关闭?
    if (pmaster->listenfd)
        close(pmaster->listenfd);

    if (pmaster->epfd)
        close(pmaster->epfd);

    free(pmaster);
    pmaster = NULL;
}

/****
1. EPOLLIN 新连接到来或者服务端内核层面收到数据通知
2. EPOLLOUT 服务端内核层面可以写通知
3. EPOLLERR/EPOLLHUP 读写非工作状态下的sockfd，客户端非正常关闭，断连接
4. EPOLLRDHUP 客户端正常关闭(close || ctrl+c)， 同时也会触发epolln事件
***/

void master_loop(master_t pmaster)
{
    int nfds = 0;
    int sockfd = -1;
    int timeout = 100;
    struct epoll_event evs[4096];
    int i;

    while (1)
    {
        nfds = epoll_wait(pmaster->epfd, evs, 4096, timeout);

        if (nfds == -1)
        {
            if (errno == EINTR)
                continue;

            print_log(LOG_TYPE_ERR, "epoll_wait error, epfd = %d, errno = %d", pmaster->epfd, errno);
            break;
        }

        for (i = 0; i < nfds; i++)
        {
            sockfd = evs[i].data.fd;

            if (sockfd == pmaster->listenfd)
            {
                fs_accept(pmaster);
                continue;
            }
            if (evs[i].events & EPOLLIN)
            {
                channel_handle_read(pmaster, sockfd);
            }
            if (evs[i].events & EPOLLOUT)
            {
                channel_handle_write(pmaster, sockfd, "hello world");
            }
            if ((evs[i].events & EPOLLERR) || (evs[i].events & EPOLLHUP))
            {
                print_log(LOG_TYPE_DEBUG, "EPOLLERR or EPOLLHUP occure");

                master_add_fd(pmaster, sockfd, EPOLL_CTL_DEL);
                close(sockfd);
            }
            if (evs[i].events & EPOLLRDHUP)
            {
                print_log(LOG_TYPE_DEBUG, "EPOLLRDHUP occure");

                master_add_fd(pmaster, sockfd, EPOLL_CTL_DEL);
                close(sockfd);
            }
        }
    }
}

void master_add_fd(master_t pmaster, int fd, int op)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    
    epoll_ctl(pmaster->epfd, op, fd, &ev);
}

void master_mod_fd(master_t pmaster, int fd, int op)
{
    print_log(LOG_TYPE_DEBUG, "Write out event");

    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP | EPOLLOUT;
    
    epoll_ctl(pmaster->epfd, op, fd, &ev);
}

void fs_accept(master_t pmaster)
{
    struct sockaddr_in *cli_addr = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));
    socklen_t cli_len = sizeof(cli_addr);
    int sockfd = -1;

    //ET模式，需要不断读取listenfd, 直到为-1且errno为EAGIN

    while (1)
    {
        sockfd = fsock_accept(pmaster->listenfd, cli_addr, cli_len);

        if (sockfd <= 0)
        {
            MEM_FREE(cli_addr);
            return;
        }
        
        pmaster->accept_count++;
        setnonblock(sockfd);
        master_add_fd(pmaster, sockfd, EPOLL_CTL_ADD);

        char *remote_ip = inet_ntoa(cli_addr->sin_addr);
        unsigned short remote_port = cli_addr->sin_port;

        print_log(LOG_TYPE_DEBUG, "Get client from %s:%u", remote_ip, remote_port);
    }
    
    MEM_FREE(cli_addr);
}

void channel_handle_read(master_t pmaster, int sockfd)
{
    char buf[CONN_READ_BUF_SIZE] = {0};
    char extra[CONN_READ_BUF_SIZE] = {0};
    struct iovec vec[2];

    vec[0].iov_base = buf;
    vec[0].iov_len = CONN_READ_BUF_SIZE;
    vec[1].iov_base = extra;
    vec[1].iov_len = CONN_READ_BUF_SIZE;
    int ret = readv(sockfd, vec, 2);

    if (ret <= 0)
    {
        if (errno != EAGAIN)
            print_log(LOG_TYPE_ERR, "Read Msg error errno is %d", errno);
            
        return;
    }

    print_log(LOG_TYPE_DEBUG, "Read client msg %s", buf);

    channel_handle_write(pmaster, sockfd, buf);
}

void channel_handle_write(master_t pmaster, int sockfd, char *buf)
{
    int len = strlen(buf);
    int ret = write(sockfd, buf, len);

    if (ret < 0)
    {
        if (errno == EAGAIN)
            master_mod_fd(pmaster, sockfd, EPOLL_CTL_MOD);
    }
    else
    {
        if (ret == len)
        {
            print_log(LOG_TYPE_DEBUG, "Write client msg over");
            close(sockfd);
        }
        else
        {
            master_mod_fd(pmaster, sockfd, EPOLL_CTL_MOD);
        }
    }
}

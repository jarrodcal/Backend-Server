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
                channel_handle_read(sockfd);
            }

            if (evs[i].events & EPOLLOUT)
            {
                channel_handle_write(sockfd);
            }

            if ((evs[i].events & EPOLLERR) || (evs[i].events & EPOLLHUP))
            {
                print_log(LOG_TYPE_DEBUG, "some one drop");
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
    ev.events = EPOLLIN | EPOLLET;
    
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
        memset(cli_addr, 0, cli_len);
        sockfd = fsock_accept(pmaster->listenfd, cli_addr, cli_len);

        if (sockfd > 0)
        {
            setnonblock(sockfd);
            master_add_fd(pmaster, sockfd, EPOLL_CTL_ADD);

            char *remote_ip = inet_ntoa(cli_addr->sin_addr);
            unsigned short remote_port = cli_addr->sin_port;

            print_log(LOG_TYPE_DEBUG, "Get client from %s:%u", remote_ip, remote_port);
        }
    }

    if (cli_addr != NULL)
    {
        free(cli_addr);
        cli_addr = NULL;
    }
}

static void channel_handle_read(int sockfd)
{
    char buf[CONN_READ_BUF_SIZE] = {0};
    char extra[CONN_READ_BUF_SIZE] = {0};
    struct iovec vec[2];

    vec[0].iov_base = buf;
    vec[0].iov_len = CONN_READ_BUF_SIZE;
    vec[1].iov_base = extra;
    vec[1].iov_len = CONN_READ_BUF_SIZE;
    ret = readv(sockfd, vec, 2);

    if (ret <= 0)
    {
        print_log("LOG_TYPE_ERR", "Read Msg error errno is %d", errno);
        return;
    }

    print_log(LOG_TYPE_DEBUG, "Get client msg %s", buf);

    channel_handle_write(sockfd, buf);
}

static void channel_handle_write(int sockfd, char *buf)
{
    int len = strlen(buf);
    int ret = write(sockfd, buf, len);

    if (ret < 0)
    {
        if (errno == EAGAIN)
        {
            struct epoll_event ev;
            ev.data.fd = fd;
            ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    
            epoll_ctl(pmaster->epfd, EPOLL_CTL_MOD, sockfd, &ev);
        }
    }
    else
    {
        if (ret == len)
        {
            print_log(LOG_TYPE_DEBUG, "Send client msg over");
        }
        else
        {
            struct epoll_event ev;
            ev.data.fd = fd;
            ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
    
            epoll_ctl(pmaster->epfd, EPOLL_CTL_MOD, sockfd, &ev);
        }

    }
}

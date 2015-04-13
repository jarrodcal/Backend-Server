#include "master.h"

master_t master_create()
{
    master_t pmaster = (master_t)malloc(sizeof(master));

    if (pmaster == NULL)
    {
        printf("malloc master error\n");
        return NULL;
    }

    pmaster->tid = pthread_slef();
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

            //客户端读写逻辑fd
        }
    }
}

void master_add_fd(master_t pmaster, int fd)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;

    epoll_ctl(pmaster->epfd, EPOLL_CTL_ADD, fd, &ev);
}

void fs_accept(master_t pmaster)
{
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);
    int sockfd = -1;

    //ET模式，需要不断读取listenfd, 直到为-1且errno为EAGIN

    while (1)
    {
        sockfd = fsock_accept(pmaster->listenfd, cli_addr, cli_len);

        if (sockfd > 0)
        {
            setnonblock(sockfd);
            master_add_fd(pmaster, sockfd);

            char *remote_ip = inet_ntoa(cli_addr.sin_addr);
            unsigned short remote_port = cli_addr.sin_port;

            print_log(LOG_TYPE_DEBUG, "Get client from %s:%u", remote_ip, remote_port);
        }
    }
}
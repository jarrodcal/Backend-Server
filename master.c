#include "master.h"

extern worker ** g_ppworker;
extern int g_workcount;

master_t master_create()
{
    master_t pmaster = (master_t)malloc(sizeof(master));

    if (pmaster == NULL)
    {
        print_log(LOG_TYPE_ERROR, "malloc master error\n");
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
    
    if (pmaster->listenfd)
        close(pmaster->listenfd);

    if (pmaster->epfd)
        close(pmaster->epfd);

    MEM_FREE(pmaster);
}

/**************************
1. EPOLLIN 新连接到来或者服务端内核层面收到数据通知
2. EPOLLOUT 服务端内核层面可以写通知
3. EPOLLERR/EPOLLHUP 读写非工作状态下的sockfd，客户端非正常关闭，断连接
4. EPOLLRDHUP 客户端正常关闭(close || ctrl+c)， 同时也会触发epolln事件
************************/
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

            print_log(LOG_TYPE_ERROR, "epoll_wait error, epfd = %d, errno = %d", pmaster->epfd, errno);
            break;
        }

        for (i = 0; i < nfds; i++)
        {
            sockfd = evs[i].data.fd;

            if (sockfd == pmaster->listenfd)
                fs_accept(pmaster);
            else
                print_log(LOG_TYPE_ERROR, "Master get non listen socket");
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

//选择一个工作线程，量小可以随机，量大后需要按照一定的负载策略
static worker_t find_worker()
{
    struct timespec timeval={0, 0};
    clock_gettime(CLOCK_REALTIME, &timeval);

    srand(timeval.tv_nsec);
    int index = rand() % 8;

    /***
    int i = 0;
    int min = 0;
    int cur = 0;
    int index = 0;
    
    for (; i<g_workcount; i++)
    {
        cur = g_ppworker[i]->total_count - g_ppworker[i]->closed_count - g_ppworker[i]->neterr_count;

        if (cur < min)
            index = i;
    }

    ***/

    return g_ppworker[index];
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
        worker_t pworker = find_worker();

        //TODO放在工作线程中去处理
        setnonblock(sockfd);
        char *remote_ip = inet_ntoa(cli_addr->sin_addr);
        unsigned short remote_port = cli_addr->sin_port;

        connector_t pconn = connector_create(sockfd, pworker, CONN_TYPE_CLIENT, remote_ip, remote_port);
        connector_sig_read(pconn);
        pworker->total_count++;
        
        print_log(LOG_TYPE_DEBUG, "Get client from %s:%u", remote_ip, remote_port);
    }
}

#include "worker.h"

extern woker ** g_ppwoker;
extern int g_workcount;

worker_t worker_create();
{
    worker_t pworker = (worker_t)malloc(sizeof(worker));

    if (pworker == NULL)
    {
        print_log(LOG_TYPE_ERR, "malloc worker error\n");
        return NULL;
    }

    pworker->tid = pthread_self();
    pworker->epfd = -1;;
    pworker->total_count = 0;
    pworker->closed_count = 0;

    pworker->epfd = epoll_create(256);

    return pworker;
}

void worker_close(worker_t pworker)
{

}

void * worker_loop(void *param)
{
    worker_t pworker = (worker_t)param;

    int nfds = 0;
    int sockfd = -1;
    int timeout = 100;
    struct epoll_event evs[4096];
    int i;

    while (1)
    {
        nfds = epoll_wait(pworker->epfd, evs, 4096, timeout);

        if (nfds == -1)
        {
            if (errno == EINTR)
                continue;

            print_log(LOG_TYPE_ERR, "worker epoll_wait error, epfd = %d, errno = %d", pworker->epfd, errno);
            break;
        }

        for (i = 0; i < nfds; i++)
        {
            sockfd = evs[i].data.fd;
            
            if (evs[i].events & EPOLLIN)
            {
                channel_handle_read(pworker, sockfd);
            }
            if (evs[i].events & EPOLLOUT)
            {
                channel_handle_write(pworker, sockfd, "hello world");
            }
            if ((evs[i].events & EPOLLERR) || (evs[i].events & EPOLLHUP))
            {
                print_log(LOG_TYPE_DEBUG, "EPOLLERR or EPOLLHUP occure");
                pworker->closed_count++;

                worker_del_fd(pworker, sockfd, EPOLL_CTL_DEL);
                close(sockfd);
            }
            if (evs[i].events & EPOLLRDHUP)
            {
                print_log(LOG_TYPE_DEBUG, "EPOLLRDHUP occure");
                pworker->closed_count++;

                worker_del_fd(pworker, sockfd, EPOLL_CTL_DEL);
                close(sockfd);
            }
        }
    }
}

void worker_add_fd(worker_t pworker, int fd, int op)
{
    struct epoll_event ev;   
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    
    epoll_ctl(pworker->epfd, op, fd, &ev);   
}

void worker_del_fd(worker_t pworker, int fd, int op)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    
    epoll_ctl(pworker->epfd, op, fd, &ev);
}

void create_worker_system(int count)
{
    int i;

    pthread_t tid;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    for (i=0; i<count; i++)
    {
        g_ppwoker[i] = worker_create();

        if (pthread_create(&tid, &attr, worker_loop, (void *)g_ppwoker[i]) != 0)
        {
            print_log(LOG_TYPE_ERR, "create work thread error");
            pthread_attr_destroy(&attr);
            return;
        }

        pthread_attr_destroy(&attr);
    }
}

//由于是ET模式，需要循环读取socket直到返回码<0且errno==EAGAIN
void channel_handle_read(worker_t pworker, int sockfd)
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

    channel_handle_write(pworker, sockfd, buf);
}

void channel_handle_write(worker_t pworker, int sockfd, char *buf)
{
    int len = strlen(buf);
    int ret = write(sockfd, buf, len);
}
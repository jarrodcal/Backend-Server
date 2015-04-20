#include "worker.h"

extern worker ** g_ppworker;
extern int g_workcount;

worker_t worker_create()
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
    int timeout = 100;
    struct epoll_event evs[4096];
    connector_t pconn = NULL;
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
            pconn = (connector_t)evs[i].data.ptr;
            
            if (evs[i].events & EPOLLIN)
            {
                worker_handle_read(pconn, evs[i].events);
            }
            if (evs[i].events & EPOLLOUT)
            {
                worker_handle_write(pconn);
            }
            if ((evs[i].events & EPOLLERR) || (evs[i].events & EPOLLHUP))
            {
                print_log(LOG_TYPE_DEBUG, "EPOLLERR or EPOLLHUP occure");
                pworker->neterr_count++;

                connector_close(pconn);
            }
            if (evs[i].events & EPOLLRDHUP)
            {
                print_log(LOG_TYPE_DEBUG, "EPOLLRDHUP occure");
                pworker->closed_count++;

                connector_close(pconn);
            }
        }
    }

    return NULL;
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
        g_ppworker[i] = worker_create();

        if (pthread_create(&tid, &attr, worker_loop, (void *)g_ppworker[i]) != 0)
        {
            print_log(LOG_TYPE_ERR, "create work thread error");
            pthread_attr_destroy(&attr);
            return;
        }

        pthread_attr_destroy(&attr);
    }
}

void worker_handle_read(connector_t pconn, int event)
{
    if (connector_read(pconn, event) > 0)
    {
        //进行消息解析和业务处理
        if (buffer_readable(pconn->preadbuf) > 0)
        {
            char *data = buffer_get_read(pconn->preadbuf);
            size_t len = strlen(data);
            buffer_read(pconn->preadbuf, len, TRUE);
            print_log(LOG_TYPE_DEBUG, "Read msg %s", data);
        
            buffer_write(pconn->pwritebuf, data, len);
            connector_write(pconn);
        }
    }
}

void worker_handle_write(connector_t pconn)
{
   connector_write(pconn);
}
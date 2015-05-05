#include "conn.h"
#include "worker.h"

inline static void connector_update(connector_t pconn, int op)
{
    struct epoll_event ev;
    ev.data.ptr = pconn;
    ev.events = pconn->evflag;

    if (pconn->pworker != NULL)
        epoll_ctl(pconn->pworker->epfd, op, pconn->sockfd, &ev);
}

connector_t connector_create(int fd, worker_t pworker, int type, char *ip, int port)
{
    connector_t pconn = (connector_t)malloc(sizeof(connector));

    if (pconn == NULL)
    {
        print_log(LOG_TYPE_ERROR, "connector malloc error. errno %d ", errno);
        return NULL;
    }

    pconn->sockfd = fd;
    pconn->evflag = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    memcpy(pconn->ip, ip, IP_LEN);
    pconn->port = port;
    pconn->type = type;
    pconn->state = CONN_STATE_NONE;

    pconn->preadbuf = buffer_create(pconn, BUFFER_SIZE);
    pconn->pwritebuf = buffer_create(pconn, BUFFER_SIZE);

    pconn->pworker = pworker;
    pconn->extra = NULL;
    memset(pconn->uid, 0, UID_MAX_LEN);

    return pconn;
}

//一直读直到读取返回值为-1，且errno=EAGIN
//由于端关闭socket时候，会触发IN和RDHUP事件，所以对于RDHUP事件要进行相应的事件记录判断, 否则读取关闭的socket会报错
int connector_read(connector_t pconn, int events)
{
    int ret = 0;
    size_t writable = 0;
    char exbuf[65536] = {0};
    struct iovec vec[2];

    if (pconn->sockfd == -1)
        return -1;

    while (1)
    {
        writable = buffer_writable(pconn->preadbuf);

        vec[0].iov_base = buffer_get_write(pconn->preadbuf);
        vec[0].iov_len = writable;
        vec[1].iov_base = exbuf;
        vec[1].iov_len = sizeof(exbuf);
        ret = readv(pconn->sockfd, vec, 2);

        if (ret < 0)
        {
            if (errno != EAGAIN)
            {
                pconn->pworker->neterr_count++;
                connector_close(pconn);
            }

            break;
        }
        else if (ret == 0)
        {
            if (!(events & EPOLLRDHUP))
            {
                pconn->pworker->neterr_count++;
                connector_close(pconn);
            }
            break;
        }
        else if (ret > 0)
        {
            if (ret <= writable)
            {
                buffer_writed(pconn->preadbuf, ret);
                break;
            }
            else
            {
                buffer_writed(pconn->preadbuf, writable);
                buffer_write(pconn->preadbuf, (char*)exbuf, ret-writable);

                if (ret < writable + sizeof(exbuf))
                    break;
            }
        }
    }

    return ret;
}

int connector_writable(connector_t pconn)
{
    if (buffer_readable(pconn->pwritebuf) > 0)
        return 1;

    return 0;
}

//只要缓冲区有数据就一直写，直到-1，并且errno=eagain，没有写完要写一次EPOOLOUT事件
int connector_write(connector_t pconn)
{
    int ret = 0;
    size_t readable = 0;

    if (pconn == NULL)
    {
        print_log(LOG_TYPE_ERROR, "connector null. errno %d ", errno);
        return -1;
    }

    while (connector_writable(pconn))
    {
        readable = buffer_readable(pconn->pwritebuf);
        ret = write(pconn->sockfd, buffer_get_read(pconn->pwritebuf), readable);

        if (ret < 0)
        {
            if (errno == EAGAIN)
            {
                connector_sig_write(pconn);
            }
            else
            {
                pconn->pworker->neterr_count++;
                connector_close(pconn);
                return -1;
            }
        }
        else if (ret <= readable)
        {
            buffer_read(pconn->pwritebuf, ret, TRUE);

            if (ret < readable)
            {
                connector_sig_write(pconn);
            }
            else
            {
                //connector_close(pconn);
                //pconn->pworker->closed_count++;
            }
        }
    }

    return 0;
}

void connector_close(connector_t pconn)
{
    if (pconn == NULL)
        return;

    if (pconn->sockfd != -1)
    {
        connector_update(pconn, EPOLL_CTL_DEL);
        close(pconn->sockfd);
        pconn->sockfd = -1;
    }

    buffer_destroy(pconn->preadbuf);
    buffer_destroy(pconn->pwritebuf);
    MEM_FREE(pconn);
}

void connector_sig_read(connector_t pconn)
{
    pconn->evflag |= EPOLLIN;
    connector_update(pconn, EPOLL_CTL_ADD);
}

void connector_sig_write(connector_t pconn)
{
    pconn->evflag |= EPOLLOUT;
    connector_update(pconn, EPOLL_CTL_ADD);
}

void connector_unsig_read(connector_t pconn)
{
    pconn->evflag &= ~EPOLLIN;
    connector_update(pconn, EPOLL_CTL_MOD);
}

void connector_unsig_write(connector_t pconn)
{
    pconn->evflag &= ~EPOLLOUT;
    connector_update(pconn, EPOLL_CTL_MOD);
}

void connector_unsig_rdhup(connector_t pconn)
{
    pconn->evflag &= ~EPOLLRDHUP;
    connector_update(pconn, EPOLL_CTL_MOD);
}

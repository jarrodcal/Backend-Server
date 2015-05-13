#include "worker.h"

extern worker ** g_ppworker;
extern int g_workcount;

worker_t worker_create()
{
    worker_t pworker = (worker_t)malloc(sizeof(worker));

    if (pworker == NULL)
    {
        print_log(LOG_TYPE_ERROR, "malloc worker error\n");
        return NULL;
    }

    pworker->total_count = 0;
    pworker->closed_count = 0;
    pworker->neterr_count = 0;
    pworker->epfd = epoll_create(256);

    hash_table *ht = (hash_table *)malloc(sizeof(hash_table));
    pworker->pht = ht;
    ht_init(pworker->pht, HT_VALUE_CONST, 0.05);

    pworker->redis = connector_create(INVALID_ID, pworker, CONN_TYPE_REDIS, REDIS_IP, REDIS_PORT);
    pworker->plist = list_create();

    struct timeval tm_now;
    gettimeofday(&tm_now, NULL);
    pworker->ticktime = tm_now.tv_sec;

    return pworker;
}

void worker_close(worker_t pworker)
{
    connector_close(pworker->redis);
    ht_destroy(pworker->pht);
    list_free(pworker->plist);
}

static void connect_redis_done(connector_t pconredis)
{
    int error = 0;
    socklen_t len = sizeof(int);

    if ((getsockopt(pconredis->sockfd, SOL_SOCKET, SO_ERROR, &error, &len)) == 0)
    {
        if (error == 0)
        {
            connector_sig_read(pconredis);
            connector_unsig_write(pconredis);
            pconredis->state = CONN_STATE_RUN;
        }
        else
        {
            connector_close(pconredis);
            print_log(LOG_TYPE_ERROR, "connect redis error, ip %s, port %d, file = %s, line = %d", pconredis->ip, pconredis->port, __FILE__, __LINE__);
        }
    }
}

static void reids_heartbeat(connector_t pconredis)
{
    struct timeval tm_now;
    gettimeofday(&tm_now, NULL);

    if ((tm_now.tv_sec - pconredis->pworker->ticktime) < REDIS_IDLETIME)
        return;

    pconredis->pworker->ticktime  = tm_now.tv_sec;

    char *key = REDIS_HBKEY;
    char cmd[REDIS_CMD_LEN] = {0};

    if (make_cmd(cmd, REDIS_CMD_LEN, 2, "get", key) < 0)
    {
        print_log(LOG_TYPE_ERROR, "get %s error, file = %s, line = %d", key, __FILE__, __LINE__);
        return;
    }

    int len = strlen(cmd);
    buffer_write(pconredis->pwritebuf, cmd, len);
    connector_write(pconredis);
}

static void connect_redis(connector_t pconredis)
{
    int ret = 0;
    int sockfd = INVALID_ID;
    struct sockaddr_in redis_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        print_log(LOG_TYPE_ERROR, "socket, file = %s, line = %d", __FILE__, __LINE__);
        return;
    }

    setnonblock(sockfd);
    setreuse(sockfd);
    set_tcp_fastclose(sockfd);

    bzero(&redis_addr, sizeof(redis_addr));
    redis_addr.sin_family = AF_INET;
    redis_addr.sin_port = htons(pconredis->port);

    if (inet_pton(AF_INET, pconredis->ip, &redis_addr.sin_addr) < 0)
    {
        close(sockfd);
        print_log(LOG_TYPE_ERROR, "inet_pton, file = %s, line = %d", __FILE__, __LINE__);
        return;
    }

    ret = connect(sockfd, (struct sockaddr *)&redis_addr, sizeof(redis_addr));

    if (ret == 0)
    {
        pconredis->sockfd = sockfd;
        connector_sig_read(pconredis);
        pconredis->state = CONN_STATE_RUN;
    }
    else if (errno == EINPROGRESS)
    {
        pconredis->sockfd = sockfd;
        connector_sig_write(pconredis);
        pconredis->state = CONN_STATE_CONNECTING;
    }
    else
    {
        close(sockfd);
        print_log(LOG_TYPE_ERROR, "connect error, file = %s, line = %d", __FILE__, __LINE__);
    }
}

//定时检查资源的长连接
void handle_time_check(worker_t pworker)
{
    connector_t pconredis = pworker->redis;

    if (pconredis->state == CONN_STATE_NONE || pconredis->state == CONN_STATE_CLOSED)
        connect_redis(pconredis);
    else
        reids_heartbeat(pconredis);
}


//这种异步非阻塞的模式，带来高性能的同时需要开设空间保存还在等待异步返回的数据，如：redis回调的顺序链表，保存connector的哈希表
void * worker_loop(void *param)
{
    worker_t pworker = (worker_t)param;
    pworker->tid = pthread_self();

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

            print_log(LOG_TYPE_ERROR, "worker epoll_wait error, epfd = %d, errno = %d", pworker->epfd, errno);
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
                print_log(LOG_TYPE_DEBUG, "EPOLLERR Or EPOLLHUP Event Occure");
                pworker->neterr_count++;

                connector_close(pconn);
            }
            if (evs[i].events & EPOLLRDHUP)
            {
                connector_unsig_read(pconn);
                connector_unsig_rdhup(pconn);

                //可以在应用层面（写缓冲区）检查数据是否已经完全发出，server发出去，系统层面会在close后根据SO_LINGER的设置处理
                print_log(LOG_TYPE_DEBUG, "EPOLLRDHUP Event Occure");
                pworker->closed_count++;

                if (buffer_readable(pconn->pwritebuf) > 0)
                    connector_write(pconn);
                else
                    connector_close(pconn);
            }

        }

        handle_time_check(pworker);
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
            print_log(LOG_TYPE_ERROR, "create work thread error");
            pthread_attr_destroy(&attr);
            return;
        }

        pthread_attr_destroy(&attr);
    }
}

void worker_handle_read(connector_t pconn, int event)
{
    switch (pconn->type)
    {
        case CONN_TYPE_CLIENT:
            channel_handle_client_read(pconn, event);
            break;

        case CONN_TYPE_REDIS:
            channel_handle_redis_read(pconn, event);
            break;
    }
}

void worker_handle_write(connector_t pconn)
{
    switch (pconn->type)
    {
        case CONN_TYPE_CLIENT:
            channel_handle_client_write(pconn);
            break;

        case CONN_TYPE_REDIS:
            channel_handle_redis_write(pconn);
            break;
    }
}

static void get_request_str(char *uid, char *cmd)
{
    int len = strlen(uid);
    char *msg = uid;
    char index[2] = {0};
    memcpy(index, msg+len-2, 1);
    char key[18] = {0};
    snprintf(key, 18, "contact_upload_%s", index);

    char *field = uid;

    if (make_cmd(cmd, REDIS_CMD_LEN, 3, "hget", key, field) < 0)
    {
        print_log(LOG_TYPE_ERROR, "hget %s %s error, file = %s, line = %d", key, field, __FILE__, __LINE__);
        return;
    }
}

//$10\r\n1984467097
static int get_client_msg(char *clientdata, message_t msg)
{
    char ch = *clientdata;

    if (ch == BULKREPLY)
    {
        char *r = strchr(clientdata, '\r');
        int length = r - clientdata - 1;
        char mm[10] = {0};
        memcpy(mm, clientdata+1, length);
        msg->len = atoi(mm);

        memcpy(msg->uid, r+2, msg->len);
        int alllen = msg->len + strlen(mm) + 3;

        print_log(LOG_TYPE_DEBUG, "Uid %s, Len %d", msg->uid, msg->len);

        return alllen;
    }

    return 0;
}

void channel_handle_client_read(connector_t pconn, int event)
{
    if (connector_read(pconn, event) > 0)
    {
        char *val = buffer_get_read(pconn->preadbuf);
        message_t pmsg = (message_t)malloc(sizeof(message));
        memset(pmsg, 0, sizeof(pmsg));
        size_t len1 = get_client_msg(val, pmsg);

        if (len1 == 0)
        {
            print_log(LOG_TYPE_ERROR, "Read Client Msg Error %s", val);
            free(pmsg);
            return;
        }

        char data[20] = {0};
        memcpy(data, pmsg->uid, pmsg->len);
        buffer_read(pconn->preadbuf, len1, TRUE);

        memcpy(pconn->uid, data, pmsg->len);

        int len2 = sizeof(connector_t);
        ht_insert(pconn->pworker->pht, data, (pmsg->len)+1, pconn, len2+1);

        context_t pcontext = (context_t)malloc(sizeof(context));
        memset(pcontext, 0, sizeof(context));
        memcpy(pcontext->data, data, pmsg->len);
        list_push_tail(pconn->pworker->plist, pcontext);

        //print_log(LOG_TYPE_DEBUG, "Hash key %s, Len %d", pcontext->data, pmsg->len);

        char cmd[REDIS_CMD_LEN] = {'\0'};
        get_request_str(data, cmd);

        int len = strlen(cmd);

        if (pconn->pworker->redis->state == CONN_STATE_RUN)
        {
            buffer_write(pconn->pworker->redis->pwritebuf, cmd, len);
            connector_write(pconn->pworker->redis);
        }
        else
        {
            print_log(LOG_TYPE_ERROR, "Redis not run");
            list_pop_head(pconn->pworker->plist);
            ht_remove(pconn->pworker->pht, data, (pmsg->len)+1);
            pconn->pworker->neterr_count++;
        }

        free(pmsg);
    }
}

static void get_response_str(char *data, char *uid, char *gdid)
{
    char *send = data;
    int len1 = strlen(uid);
    memcpy(send, uid, len1);
    memcpy(send+len1, "$", 1);

    int len2 = strlen(send);
    int len3 = strlen(gdid);
    memcpy(send+len2, gdid, len3);

    print_log(LOG_TYPE_DEBUG, "Send Client %s", data);
}

void channel_handle_redis_read(connector_t pconn, int event)
{
    if (connector_read(pconn, event) > 0)
    {
        char *origin = buffer_get_read(pconn->preadbuf);
        char analyse[100] = {0};
        int originlen = get_analyse_data(origin, analyse);
        buffer_read(pconn->preadbuf, originlen, TRUE);

        if (strcmp(analyse, REDIS_HBVAL) == 0)
            return;

        context_t pcontext = (context_t)pconn->pworker->plist->head->value;
        //print_log(LOG_TYPE_DEBUG, "Redis Read %s List Head Uid %s", analyse, pcontext->data);

        char key[UID_MAX_LEN] = {0};
        memcpy(key, pcontext->data, strlen(pcontext->data));
        MEM_FREE(pcontext);
        list_pop_head(pconn->pworker->plist);

        size_t len1 = strlen(key);
        size_t len2 = 0;
        connector_t pclientcon = (connector_t)ht_get(pconn->pworker->pht, key, len1+1, &len2);

        if (pclientcon)
        {
            ht_remove(pconn->pworker->pht, key, len1+1);

            char val[100] = {0};
            get_response_str(val, key, analyse);
            size_t size = strlen(val);

            buffer_write(pclientcon->pwritebuf, val, size);
            connector_write(pclientcon);
        }
    }
}

void channel_handle_client_write(connector_t pconn)
{
    connector_unsig_write(pconn);
    connector_write(pconn);
}

void channel_handle_redis_write(connector_t pconn)
{
    //以非阻塞模式设置套接口，去连接Redis，Epoll返回写通知事件，可能是有数据也可能是连接状态的返回。
    //通过con的state判断是哪种情况，通过调用getsockopt来查看返回的连接状态（连接成功和失败都会返回可写通知）
    if (pconn->state == CONN_STATE_CONNECTING || pconn->state == CONN_STATE_CLOSED)
    {
        connect_redis_done(pconn);
    }
    else
    {
        connector_unsig_write(pconn);
        connector_write(pconn);
    }
}
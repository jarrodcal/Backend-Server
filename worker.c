/********************************
@功能: 客户端输入uid，服务端输出uid$gdid
@流程: 每一次客户端请求创建一个连接，并保存在哈希表中，构造reids协议异步请求，获得数据后返回给客户端
@例子：输入：123249943, 输出：123249943$adfa899ad2，返回uid同时是为了便于客户端进行数据校验
**********************************/

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

    //初始fd为-1，表示资源的长连接未建立或者资源掉线
    pworker->redis = connector_create(INVALID_ID, pworker, CONN_TYPE_REDIS, REDIS_IP, REDIS_PORT);
    pworker->plist = list_create();

    return pworker;
}

void worker_close(worker_t pworker)
{
    //redis连接关闭
    //hash表的资源释放
    //连接关闭
    //ht_destroy
    //list
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
    //失败的情况下设置连接状态pconredis->state == CONN_STATE_CLOSED;
    char *key = "hbkey";
    char cmd[REDIS_CMD_LEN] = {'\0'};

    if (make_cmd(cmd, REDIS_CMD_LEN, 2, "get", key) < 0)
    {
        print_log(LOG_TYPE_ERROR, "get %s error, file = %s, line = %d", key, __FILE__, __LINE__);
        return;
    }

    context_t pcontext = (context_t)malloc(sizeof(context));
    memcpy(pcontext->uid, key, strlen(key));
    list_push_tail(pconredis->pworker->plist, pcontext);

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

//资源的定时检查, 断开后会重新连接
void handle_time_check(worker_t pworker)
{
    connector_t pconredis = pworker->redis;

    if (pconredis->state == CONN_STATE_NONE || pconredis->state == CONN_STATE_CLOSED)
        connect_redis(pconredis);
    else
        reids_heartbeat(pconredis);

    //定时查看哈希表中的连接，超过一定时间，如果对方没有关闭，需要关闭
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
                print_log(LOG_TYPE_DEBUG, "EPOLLERR or EPOLLHUP occure");
                pworker->neterr_count++;

                connector_close(pconn);
            }
            if (evs[i].events & EPOLLRDHUP)
            {
                //可以在应用层面（写缓冲区）检查数据是否已经完全发出，server发出去，系统层面会在close后根据SO_LINGER进行相关的处理
                print_log(LOG_TYPE_DEBUG, "EPOLLRDHUP occure");
                pworker->closed_count++;

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

void channel_handle_client_read(connector_t pconn, int event)
{
    if (connector_read(pconn, event) > 0)
    {
        //进行消息解析和业务处理
        if (buffer_readable(pconn->preadbuf) > 0)
        {
            char *data = buffer_get_read(pconn->preadbuf);
            size_t len = strlen(data);
            buffer_read(pconn->preadbuf, len, TRUE);
            memcpy(pconn->uid, data, len);

            print_log(LOG_TYPE_DEBUG, "Read From Client %s, len is %d", data, len);

            //保存连接到哈希表
            int len2 = sizeof(connector_t);
            ht_insert(pconn->pworker->pht, data, len+1, pconn, len2+1);

            //将请求放到链表中
            context_t pcontext = (context_t)malloc(sizeof(context));
            memset(pcontext, 0, sizeof(context));
            memcpy(pcontext->uid, data, len);
            list_push_tail(pconn->pworker->plist, pcontext);

            print_log(LOG_TYPE_DEBUG, "Hash table key is  %s", pcontext->uid);;

            char *msg = data;
            char index[2] = {0};
            memcpy(index, msg+len-2, 1);
            char key[18] = {0};
            snprintf(key, 18, "contact_upload_%s", index);

            char *field = data;
            char cmd[REDIS_CMD_LEN] = {'\0'};

            if (make_cmd(cmd, REDIS_CMD_LEN, 3, "hget", key, field) < 0)
            {
                print_log(LOG_TYPE_ERROR, "hget %s %s error, file = %s, line = %d", key, field, __FILE__, __LINE__);
                return;
            }

            len = strlen(cmd);

            if (pconn->pworker->redis->state == CONN_STATE_RUN)
            {
                buffer_write(pconn->pworker->redis->pwritebuf, cmd, len);
                connector_write(pconn->pworker->redis);
            }
            else
            {
                print_log(LOG_TYPE_ERROR, "Redis not run");
            }
        }
    }
}

void channel_handle_redis_read(connector_t pconn, int event)
{
    connector_read(pconn, event);

    if (buffer_readable(pconn->preadbuf) > 0)
    {
        char *origindata = buffer_get_read(pconn->preadbuf);

        if (strstr(origindata, "hbval") == NULL)
            print_log(LOG_TYPE_DEBUG, "Read From Redis %s", origindata);

        char analysedata[100] = {0};
        int originlen = get_analyse_data(origindata, analysedata);
        buffer_read(pconn->preadbuf, originlen, TRUE);

        size_t value_size;

        //多级指向要付个值且先判断下..... if == NULL
        //取出链表请求的头元素, 找到对应请求的uid
        context_t pcontext = (context_t)pconn->pworker->plist->head->value;

        if (strstr(pcontext->uid, "hbkey") == NULL)
            print_log(LOG_TYPE_DEBUG, "List Head Uid %s", pcontext->uid);

        char key[32] = {0};
        memcpy(key, pcontext->uid, strlen(pcontext->uid));

        MEM_FREE(pcontext);
        list_pop_head(pconn->pworker->plist);

        int len = strlen(key);
        connector_t pclientcon = (connector_t)ht_get(pconn->pworker->pht, key, len+1, &value_size);

        //只存储一次连接
        if (pclientcon)
        {
            ht_remove(pconn->pworker->pht, key, len+1);

            if (strlen(analysedata) > 0)
            {
                char senddata[100] = {0};
                memcpy(senddata, key, len);
                memcpy(senddata+len, "$", 1);

                len = strlen(senddata);
                int size = strlen(analysedata);
                memcpy(senddata+len, analysedata, size);

                len = strlen(senddata) + 1;

                print_log(LOG_TYPE_DEBUG, "Send Client %s", senddata);

                buffer_write(pclientcon->pwritebuf, senddata, len);
            }
            
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
    //以非阻塞模式设置套接口，去connect Redis，返回的套接字是可写状态，此时Epoll返回写通知事件，可能是有数据也可能是连接状态的返回。
    //通过con的state判断是哪种情况，通过调用getsockopt来得到返回的连接状态（连接成功和失败都会返回可写通知）

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
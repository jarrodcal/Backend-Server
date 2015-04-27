#ifndef __WORKER_H__
#define __WORKER_H__

#include "common.h"
#include "log.h"
#include "conn.h"
#include "buffer.h"
#include "hashtable.h"
#include "redis.h"
#include "list.h"

//哈希表存放建立起的连接，以uid为key，value为连接指针
//redis变量为业务上和资源的长连接

struct worker
{
    pthread_t tid;
    int epfd;
    int waitfd_count;
    int waitfd[WAITFD_COUNT];
    int total_count;
    int closed_count;
    int neterr_count;
    connector_t redis;
    hash_table *pht;
    list_t plist;
};

typedef struct worker worker;
typedef struct worker * worker_t;

worker_t worker_create();
void worker_close(worker_t pworker);
void *worker_loop(void *param);
void handle_time_check(worker_t pworker);
void create_worker_system(int count);

void worker_handle_read(connector_t pconn, int event);
void worker_handle_write(connector_t pconn);

void channel_handle_client_read(connector_t pconn, int event);
void channel_handle_client_write(connector_t pconn);
void channel_handle_redis_read(connector_t pconn, int event);
void channel_handle_redis_write(connector_t pconn);

void handle_time_check(worker_t pworker);

#endif
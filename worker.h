#ifndef __WORKER_H__
#define __WORKER_H__

#include "common.h"
#include "log.h"
#include "conn.h"
#include "buffer.h"
#include "hashtable.h"
#include "redis.h"
#include "list.h"

struct worker
{
    pthread_t tid;
    int epfd;
    int total_count;
    int closed_count;
    int neterr_count;
    connector_t redis;
    hash_table *pht;
    list_t plist;
};

typedef struct worker worker;
typedef struct worker * worker_t;

struct message
{
    char uid[UID_MAX_LEN];
    int len;
};

typedef struct message message;
typedef struct message * message_t;

void create_worker_system(int count);
worker_t worker_create();
void *worker_loop(void *param);
void worker_close(worker_t pworker);

void worker_handle_read(connector_t pconn, int event);
void worker_handle_write(connector_t pconn);

void channel_handle_client_read(connector_t pconn, int event);
void channel_handle_client_write(connector_t pconn);
void channel_handle_redis_read(connector_t pconn, int event);
void channel_handle_redis_write(connector_t pconn);

void handle_time_check(worker_t pworker);

#endif
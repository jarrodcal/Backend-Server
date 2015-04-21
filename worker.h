#ifndef __WORKER_H__
#define __WORKER_H__

#include "common.h"
#include "log.h"
#include "conn.h"
#include "buffer.h"
#include "hashtable.h"

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
};

typedef struct worker worker;
typedef struct worker * worker_t;

worker_t worker_create();
void worker_close(worker_t pworker);
void *worker_loop(void *param);
void create_worker_system(int count);

void worker_handle_read(connector_t pconn, int event);
void worker_handle_write(connector_t pconn);

#endif
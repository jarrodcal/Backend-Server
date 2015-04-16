#ifndef __WORKER_H__
#define __WORKER_H__

#include "common.h"
#include "log.h"

struct worker
{
    pthread_t tid;
    int epfd;
    int total_count;
    int closed_count;
};

typedef struct worker worker;
typedef struct worker * worker_t;

worker_t worker_create();
void worker_close(worker_t pworker);
void worker_loop(worker_t pworker);
void create_worker_system(int count);

void worker_add_fd(worker_t pworker, int fd, int op);
void worker_del_fd(worker_t pworker, int fd, int op);

void channel_handle_read(worker_t pworker, int sockfd);
void channel_handle_write(worker_t pworker, int sockfd, char *buf);

#endif
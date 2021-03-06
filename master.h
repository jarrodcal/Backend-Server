#ifndef __MASTER_H__
#define __MASTER_H__

#include "common.h"
#include "log.h"
#include "worker.h"
#include "conn.h"

struct master
{
    pthread_t tid;
    int epfd;
    int listenfd;
    int accept_count;
};

typedef struct master master;
typedef struct master * master_t;

master_t master_create();
void master_close(master_t pmaster);
void master_loop(master_t pmaster);
void master_add_fd(master_t pmaster, int fd, int op);
void fs_accept(master_t pmaster);

#endif
#ifndef __CONN_H__
#define __CONN_H__

#include "common.h"
#include "log.h"
#include "buffer.h"

//TODO connector pool
//结构体成员变量中不能使用typedef的定义，可以使用直接

struct connector
{
    int sockfd;
    int evflag;
    char ip[16];
    int port;
    int type;
    int state;
    buffer_t preadbuf;
    buffer_t pwritebuf;
    struct worker * pworker;
    char uid[UID_MAX_LEN];
    void *extra;
};

typedef struct connector connector;
typedef struct connector *connector_t;

connector_t connector_create(int fd, struct worker * pworker, int type, char *ip, int port);
int connector_read(connector_t pconn, int events);
int connector_write(connector_t pconn);
void connector_close(connector_t pconn);
int connector_writable(connector_t pconn);

void connector_sig_read(connector_t pconn);
void connector_sig_write(connector_t pconn);
void connector_unsig_read(connector_t pconn);
void connector_unsig_write(connector_t pconn);
void connector_unsig_rdhup(connector_t pconn);

#endif
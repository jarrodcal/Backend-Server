#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

typedef int BOOL;

#define MEM_FREE(p) if (p!=NULL) {free(p); p=NULL;}

#define BUFFER_SIZE 1024
#define UID_MAX_LEN 32
#define WAITFD_COUNT 10240
#define IP_LEN 16

#define TRUE 1
#define FALSE 0
#define INVALID_ID (-1)

#define CONN_TYPE_CLIENT 0
#define CONN_TYPE_REDIS 1
#define CONN_TYPE_MYSQL 2

#define CONN_STATE_NONE 0
#define CONN_STATE_CONNECTING 1
#define CONN_STATE_RUN 2
#define CONN_STATE_CLOSED 3

#define REDIS_IP "172.16.89.100"
#define REDIS_PORT 29023
#define REDIS_CMD_LEN 128

#define REDIS_HBKEY "hbkey"
#define REDIS_HBVAL "hbval"

void setnonblock(int sock);
void setreuse(int sock);
void set_tcp_nodelay(int sock);
void set_tcp_fastclose(int sock);
int listen_init(int* listenfd, const char* ip, unsigned short port);
int fsock_accept(int listenfd, struct sockaddr_in *cli_addr, socklen_t cli_len);

#endif

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

void setnonblock(int sock);
void setreuse(int sock);
void set_tcp_nodelay(int sock);
int listen_init(int* listenfd, const char* ip, unsigned short port);
int fsock_accept(int listenfd, struct sockaddr_in cli_addr, socklen_t cli_len);

#endif

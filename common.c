#include "common.h"
#include "log.h"

void setnonblock(int sock)
{
    int opts = fcntl(sock, F_GETFL);

    if (opts < 0)
    {
        print_log(LOG_TYPE_ERR, "fcntl(sock, GETFL) is error!");
        return;
    }

    opts = opts | O_NONBLOCK;

    if (fcntl(sock, F_SETFL, opts) < 0)
    {
        print_log(LOG_TYPE_ERR, "fcntl(sock, SETFL, opts) is error!");
        return;
    }
}

void setreuse(int sock)
{
    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
}

void set_tcp_nodelay(int sock)
{
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
}

int listen_init(int* listenfd, const char* ip, unsigned short port)
{
    struct sockaddr_in serveraddr;
    int sockfd = -1;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        print_log(LOG_TYPE_ERR, "create listen socket, file = %s, line = %d", __FILE__, __LINE__);
        return -1;
    }

    setnonblock(sockfd);
    set_tcp_nodelay(sockfd);
    setreuse(sockfd);

    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_aton(ip, &(serveraddr.sin_addr));
    serveraddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
    {
        print_log(LOG_TYPE_ERR, "bind, file = %s, line = %d", __FILE__, __LINE__);
        return -1;
    }

    if (listen(sockfd, 10240) == -1)
    {
        print_log(LOG_TYPE_ERR, "listen, file = %s, line = %d", __FILE__, __LINE__);
        return -1;
    }

    *listenfd = sockfd;

    print_log(LOG_TYPE_DEBUG, "Server up bind ip: %s and port: %u", ip, port);

    return 0;
}

int fsock_accept(int listenfd, struct sockaddr_in cli_addr, socklen_t cli_len)
{
    int sockfd = -1;

    if ((sockfd = accept(listenfd, (struct sockaddr *)&cli_addr, &cli_len)) < 0)
    {
        if (errno != ECONNABORTED || errno != EINTR || errno != EAGAIN)
            print_log(LOG_TYPE_ERR, "accept error. errno = %d", errno);
    }

    return sockfd;
}
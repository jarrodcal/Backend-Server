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

//TCP中先调用close()的一方会进入TIME_WAIT状态, 在其状态下想重新使用该端口，需要将套接字设置为SO_REUSEADDR
void setreuse(int sock)
{
    int flag = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(int));
}

//@1 : Nagle’s Algorithm 是为了提高带宽利用率设计的算法，其做法是合并小的TCP包为一个，避免了过多的小报文的TCP头所浪费的带宽。如果开启了这个算法(默认)则协议栈会累积数据直到以下两个条件之一才发出：积累的数据量到达最大的 TCP Segment Size 或者收到了一个Ack
//@2: TCP Delay Acknoledgement策略是延迟Ack包的发送，使得协议栈有机会合并多个Ack，提高网络性能。40ms超时会进行补发

//http://jerrypeng.me/2013/08/mythical-40ms-delay-and-tcp-nodelay/

//解决方案1： 减少write小的数据包，合并到一起进行发送，业务单元的发送要完整的发出去不要进行分拆
//解决方案2： TCP_NODELAY socket的设置，避免双方都等待到40ms的情况，只要有一端不等待就行

void set_tcp_nodelay(int sock)
{
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(int));
}

//不采用内核的延迟关闭(会造成time_wait状态，根据情况将遗留在缓冲区的数据进行发送)，下面函数直接发送一个RST报文
void set_tcp_fastclose(int sock)
{
    struct linger ln = {1, 0};
    setsockopt(sock, SOL_SOCKET, SO_LINGER, &ln, sizeof(ln));
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

//sockaddr_in 和 sockaddr 两种结构体的大小是一致的
//ECONNABORTED  client send rst after three times handshake
//EAGAIN fd can not read currently
//EINTER system intr
int fsock_accept(int listenfd, struct sockaddr_in *cli_addr, socklen_t cli_len)
{
    int sockfd = -1;

    if ((sockfd = accept(listenfd, (struct sockaddr *)cli_addr, &cli_len)) < 0)
    {
        if ((errno != EAGAIN) && (errno != ECONNABORTED) && (errno != EINTR))
            print_log(LOG_TYPE_ERR, "accept error. errno = %d", errno);
    }

    return sockfd;
}

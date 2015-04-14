#include "log.h"
#include "master.h"
//#include "slave.h"

int main()
{
    const char *path = "/data1/echo-server/";
    init_log_system(path);

    register_log_type(LOG_TYPE_ERR, "err.log");
    register_log_type(LOG_TYPE_DEBUG, "debug.log");

    const char *ip = "172.16.38.26";
    unsigned short port = 9432;

    int listenfd = -1;

    if (listen_init(&listenfd, ip, port) == -1)
    {
        printf("bind error: %s, %d\n", ip, port);
        exit(1);
    }

    master_t pmaster = master_create();
    pmaster->listenfd = listenfd;
    master_add_fd(pmaster, listenfd, EPOLL_CTL_ADD);
    print_log(LOG_TYPE_DEBUG, "Begin listening ..., file = %s, line = %d", __FILE__, __LINE__);

    master_loop(pmaster);

    return 0;
}

#include "log.h"
#include "master.h"
#include "worker.h"
#include "status.h"

worker ** g_ppworker = NULL;
int g_workcount = 8;

static int setlimit()
{
    struct rlimit rt;

    rt.rlim_cur = 4000000;
    rt.rlim_max = 4000000;

    if (setrlimit(RLIMIT_NOFILE, &rt) == -1)
    {
        print_log(LOG_TYPE_ERR, "setrlimit fd, file = %s, line = %d", __FILE__, __LINE__);
        return -1;
    }

    rt.rlim_cur = RLIM_INFINITY;
    rt.rlim_max = RLIM_INFINITY;

    if (setrlimit(RLIMIT_CORE, &rt) == -1)
    {
        print_log(LOG_TYPE_ERR, "setrlimit core, file = %s, line = %d", __FILE__, __LINE__);
        return -1;
    }

    return 0;
}

void echo_resource_init()
{
    g_ppworker = (worker **)malloc(sizeof(worker *) * g_workcount);

    if (g_ppworker == NULL)
    {
        print_log(LOG_TYPE_ERR, "Malloc g_ppworker error");
        exit(1);
    }

    int i;

    for (i=0; i<g_workcount; i++)
        g_ppworker[i] = NULL;
}

int main()
{
    setlimit();

    const char *path = "/data1/Backend-Server-master/";
    init_log_system(path);

    register_log_type(LOG_TYPE_ERR, "err.log");
    register_log_type(LOG_TYPE_DEBUG, "debug.log");
    register_log_type(LOG_TYPE_STATUS, "status.log");

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

    echo_resource_init();
    create_status_system(pmaster);
    create_worker_system(g_workcount);

    print_log(LOG_TYPE_DEBUG, "Begin listening ..., file = %s, line = %d", __FILE__, __LINE__);

    master_loop(pmaster);

    return 0;
}

#include "status.h"
#include "common.h"
#include "log.h"

void create_status_system(master_t pmaster)
{
    pthread_t tid;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&tid, &attr, get_master_status, pmaster) != 0)
    {
        printf("create status pthread error\n");
        return -1;
    }

    pthread_attr_destroy(&attr);
}

void get_master_status(master_t pmaster)
{
    while (1)
    {
        int count = pmaster->accept_count;
        print_log(LOG_TYPE_STATUS, "Master accept count is %d ", count);
        
        sleep(2);
    }
}
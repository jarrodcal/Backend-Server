#include "status.h"
#include "common.h"
#include "log.h"

void create_status_system(master_t pmaster)
{
    pthread_t tid;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&tid, &attr, get_master_status, (void *)pmaster) != 0)
    {
        print_log(LOG_TYPE_ERR, "create status pthread error");
        pthread_attr_destroy(&attr);
        return;
    }

    pthread_attr_destroy(&attr);
}

void *get_master_status(void *param)
{
    master_t pmaster = (master_t)param;

    while (1)
    {
        int count = pmaster->accept_count;
        print_log(LOG_TYPE_STATUS, "Master accept count is %d ", count);
        
        sleep(1);
    }

    return NULL;
}
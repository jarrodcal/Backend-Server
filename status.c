#include "status.h"

extern worker ** g_ppworker;
extern int g_workcount;

void create_status_system(master_t pmaster)
{
    pthread_t tid;
    pthread_attr_t attr;
    
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&tid, &attr, get_master_status, (void *)pmaster) != 0)
    {
        print_log(LOG_TYPE_ERROR, "create status pthread error");
        pthread_attr_destroy(&attr);
        return;
    }

    pthread_attr_destroy(&attr);
}

void *get_master_status(void *param)
{
    master_t pmaster = (master_t)param;
    int i;
    int cur;

    while (1)
    {
        print_log(LOG_TYPE_STATUS, "Master accept count is %d ", pmaster->accept_count);

        for (i=0; i<g_workcount; i++)
        {
            if (g_ppworker[i])
            {
                //加锁?
                cur = g_ppworker[i]->total_count - g_ppworker[i]->closed_count - g_ppworker[i]->neterr_count;
                print_log(LOG_TYPE_STATUS, "Lpid 0x%lx Worker %d total %d, Neterr %d, Cur is %d ", g_ppworker[i]->tid, i, g_ppworker[i]->total_count, g_ppworker[i]->neterr_count, cur);
                print_log(LOG_TYPE_STATUS, "Hashtable keycount is %d array size is %d", g_ppworker[i]->pht->key_count, g_ppworker[i]->pht->array_size);
            }
        }
        
        sleep(1);
    }
}
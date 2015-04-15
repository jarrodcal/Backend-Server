#include "log.h"

static void log_write2disk(log_t plog)
{
    int size, offset = 0;
    FILE* file = NULL;
    char log_name[LOG_FULLNAME_LEN] = {0};

    struct tm t_tm;
    time_t timer;
    time(&timer);
    localtime_r(&timer, &t_tm);

    offset += snprintf(log_name, LOG_FULLNAME_LEN, "%s/%04d-%02d/", log_prefix, t_tm.tm_year + 1900, t_tm.tm_mon + 1);

    if (access(log_name, F_OK) != 0)
    {
        if (mkdir(log_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0)
            return;
    }

    offset += snprintf(log_name + offset, LOG_FULLNAME_LEN, "%02d/", t_tm.tm_mday);

    if (access(log_name, F_OK) != 0)
    {
        if (mkdir(log_name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) < 0)
            return;
    }

    strcat(log_name, plog->name);
    pthread_mutex_lock(&plog->mutex);

    if (plog->index != 0)
    {
        if ((file = fopen(log_name, "a")) != NULL)
        {
            size = fwrite(plog->buffer, 1, plog->index, file);
            plog->index -= size;
            pthread_cond_signal(&plog->cond);
            fclose(file);
        }
    }

    pthread_mutex_unlock(&plog->mutex);
}

//param *参数和返回值void *, 便于进行扩展
static void * log_daemon(void* param)
{
    int i;

    while (1)
    {
        for (i = 0; i < LOG_TYPE_MAX; i++)
        {
            if (logs[i].use == 1)
                log_write2disk(&logs[i]);
        }

        //0.05s写次日志
        usleep(1000 * 50);
    }

    return NULL;
}

//使log类型有效
void register_log_type(enum log_type type, const char *name)
{
    if (type < 0 || type >= LOG_TYPE_MAX || name == NULL)
        return;

    strncpy(logs[type].name, name, LOG_NAME_LEN);
    logs[type].use = 1;
}

int init_log_system(const char *prefix)
{
    int i;
    pthread_t tid;
    pthread_attr_t attr;

    if (prefix == NULL)
        return -1;

    strncpy(log_prefix, prefix, LOG_PREFIX_LEN);

    for (i = 0; i < LOG_TYPE_MAX; i++)
    {
        logs[i].index = 0;
        logs[i].size = LOG_BUF_SIZE;
        memset(logs[i].buffer, 0, LOG_BUF_SIZE);

        pthread_mutex_init(&(logs[i].mutex), NULL);
        pthread_cond_init(&(logs[i].cond), NULL);
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

    if (pthread_create(&tid, &attr, log_daemon, NULL) != 0)
    {
        printf("create pthread error\n");
        return -1;
    }

    pthread_attr_destroy(&attr);

    return 1;
}

void print_log(enum log_type type, const char *format, ...)
{
    char fmt[LOG_FORMAT_LEN] = {0};
    char line[LOG_LINE_LEN] = {0};

    if (type < 0 || type >= LOG_TYPE_MAX || logs[type].use != 1)
        return;

    struct tm t_tm;
    time_t timer;
    time(&timer);
    localtime_r(&timer, &t_tm);

    int len;
    log_t plog;
    va_list args;

    len = snprintf(fmt, LOG_FORMAT_LEN, "%04d-%02d-%02d %02d:%02d:%02d,%lu,%s\n", t_tm.tm_year + 1900, t_tm.tm_mon + 1, t_tm.tm_mday, t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec, (unsigned long) pthread_self(), format);

    if (len < 0 || len > LOG_FORMAT_LEN)
        return;

    va_start(args, format);
    len = vsnprintf(line, LOG_LINE_LEN, fmt, args);
    va_end(args);

    if (len < 0 || len > LOG_LINE_LEN)
        return;

    plog = &logs[type];
    pthread_mutex_lock(&plog->mutex);

    while (plog->index+len > plog->size)
        pthread_cond_wait(&plog->cond, &plog->mutex);

    strncpy(plog->buffer+plog->index, line, len);
    plog->index += len;

    pthread_mutex_unlock(&plog->mutex);
}


#ifndef _LOG_H_
#define _LOG_H_

#include "common.h"

#define LOG_PREFIX_LEN 512
#define LOG_NAME_LEN 128
#define LOG_FULLNAME_LEN (LOG_PREFIX_LEN+LOG_NAME_LEN)

#define LOG_BUF_SIZE 1024 * 1024

#define LOG_FORMAT_LEN 1024  //日志时间和一些额外信息
#define LOG_LINE_LEN 2048   //上面的信息+实际的数据

enum log_type
{
    LOG_TYPE_RUN,
    LOG_TYPE_DEBUG,
    LOG_TYPE_ERR,
    LOG_TYPE_STATUS,
    LOG_TYPE_MAX
};

//采用互斥锁和条件变量进行线程间的同步
//互斥锁使得变量同步，条件变量使得其有序

struct log
{
    int use;
    char name[LOG_NAME_LEN];
    char buffer[LOG_BUF_SIZE];
    int size;
    int index;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
};

typedef struct log log_s;
typedef struct log * log_t;

log_s logs[LOG_TYPE_MAX];
char log_prefix[LOG_PREFIX_LEN];

int init_log_system(const char *prefix);
void register_log_type(enum log_type type, const char *name);
void print_log(enum log_type type, const char *format, ...);

#endif

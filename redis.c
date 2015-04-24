#include "redis.h"

int make_cmd(char *buffer, int max_len, int count, ...)
{
    char *arg;
    va_list ap;
    int i, offset;

    offset = snprintf(buffer, max_len, "*%d\r\n", count);
    va_start(ap, count);

    for (i = 0; i <count; i++)
    {
        arg = va_arg(ap, char *);
        offset += snprintf(buffer+offset, max_len-offset, "$%d\r\n%s\r\n", (int) strlen(arg), arg);

        if (offset >= max_len)
            return -1;
    }

    va_end(ap);

    return 0;
}

int redis_on_recv_data(connector_t connector)
{
    return 0;
}
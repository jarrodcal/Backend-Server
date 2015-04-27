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

////"$6\r\nfoobar\r\n"
char *get_analyse_data(char *origindata)
{
    char ch = *origindata;
    char gdids[100] = {0};

    if (ch == BULKREPLY)
    {
        char *begin = strchr(origindata, '\n');
        char *again = strchr(begin, '\r');
        int len = again - begin - 1;
        memcpy(gdids, begin+1, len);
    }
    else
    {
        print_log(LOG_TYPE_DEBUG, "not BULKREPLY is  %s", origindata);
    }

    return gdids;
}
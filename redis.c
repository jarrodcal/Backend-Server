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
int get_analyse_data(char *origin, char *gdid)
{
    char ch = *origin;
    char * data = origin;
    int len = 0;
    int alllen = 0;

    if (ch == BULKREPLY)
    {
        char *r = strchr(origin, '\r');
        int length = r - origin - 1;
        char mm[10] = {0};
        memcpy(mm, data+1, length);
        len = atoi(mm);

        if (len < 0)
        {
            memcpy(gdid, "No", 2);
            alllen = 5;
        }
        else
        {
            memcpy(gdid, r+2, len);
            alllen = len + strlen(mm) + 5;
        }
    }
    else
    {
        memcpy(gdid, "Error", 2);
        print_log(LOG_TYPE_DEBUG, "not BULKREPLY is  %s", origin);
    }

    return alllen;
}
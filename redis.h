#include "common.h"
#include "conn.h"

#define BULKREPLY '$'

struct context
{
    char data[UID_MAX_LEN];
    void *extra;
};

typedef struct context context;
typedef struct context * context_t;

int make_cmd(char *buffer, int max_len, int count, ...);
int get_analyse_data(char *origindata, char *gdids);

#include "common.h"
#include "conn.h"

int make_cmd(char *buffer, int max_len, int count, ...);
int redis_on_recv_data(connector_t connector);

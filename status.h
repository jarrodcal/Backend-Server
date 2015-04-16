#ifndef __STATUS_H__
#define __STATUS_H__

#include "common.h"
#include "log.h"
#include "master.h"
#include "worker.h"

//查看所有master和worker运行状态的struct

void create_status_system(master_t);
void *get_master_status(void *);

#endif
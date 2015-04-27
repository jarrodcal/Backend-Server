#ifndef _LIST_H_
#define _LIST_H_

#include "common.h"
#include "log.h"

struct node
{
    void *value;
    struct node *next;
};

typedef struct node node;
typedef struct node * node_t;

struct list
{
    node_t head;
    node_t tail;
    unsigned int len;
};

typedef struct list list;
typedef struct list * list_t;

list_t list_create();
void list_free(list_t);
int list_push_tail(list_t, void *);
void list_pop_head(list_t);

#endif
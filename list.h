#ifndef _LIST_H_
#define _LIST_H_

#include <stdio.h>
#include <stdlib.h>

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
    void (*free)(void *ptr);
    unsigned int len;
};

typedef struct list list;
typedef struct list * list_t;

list_t list_create(void (*freefun)(void *value));
void list_free(list_t, node_t);
int list_push_tail(list_t, void *);
void list_pop_head(list_t, node_t);
void list_travl(list_t);

#endif
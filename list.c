#include "list.h"

list_t list_create(void (*freefun)(void *value))
{
    list_t plist = (list_t)malloc(sizeof(list));

    if (plist == NULL)
    {
        printf("malloc error\n");
        return NULL;
    }

    plist->head = NULL;
    plist->tail = NULL;
    plist->len = 0;
    plist->free = freefun;

    return plist;
}

void node_free(list_t plist, node_t pnode)
{
    if (plist->free)
    {
        (plist->free)(pnode->value);
        free(pnode);
    }
}

int list_push_tail(list_t plist, void *value)
{
    if (plist == NULL)
        return 0;

    node_t pnode = (node_t)malloc(sizeof(node));

    if (pnode == NULL)
    {
        printf("malloc error\n");
        return 0;
    }

    pnode->value = value;
    pnode->next = NULL;
    
    if (plist->head == NULL)
    {
        plist->head = plist->tail = pnode;
    }
    else
    {
        plist->tail->next = pnode;
        plist->tail = pnode;
    }

    plist->len++;

    return 1;
}

void list_pop_head(list_t plist, node_t pnode)
{
    if ((plist == NULL) || (plist->head == NULL))
        return NULL;

    pnode = plist->head;
    plist->head = plist->head->next;
    plist->len--;
}

void list_travl(list_t plist)
{
    node_t pnode = plist->head;

    while (pnode)
    {
        void *nodeval = pnode->value;
        pnode = pnode->next;
    }
}
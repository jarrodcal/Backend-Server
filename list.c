#include "list.h"

list_t list_create()
{
    list_t plist = (list_t)malloc(sizeof(list));

    if (plist == NULL)
    {
        print_log(LOG_TYPE_ERROR, "list_create error file = %s, line = %d", __FILE__, __LINE__);
        return NULL;
    }

    plist->head = NULL;
    plist->tail = NULL;
    plist->len = 0;

    return plist;
}

void node_free(list_t plist)
{
    if (plist->len > 0)
        list_pop_head(plist);

    free(plist);
}

int list_push_tail(list_t plist, void *value)
{
    if (plist == NULL)
        return 0;

    node_t pnode = (node_t)malloc(sizeof(node));

    if (pnode == NULL)
    {
        print_log(LOG_TYPE_ERROR, "list node creaet error file = %s, line = %d", __FILE__, __LINE__);
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

void list_pop_head(list_t plist)
{
    if ((plist == NULL) || (plist->head == NULL))
        return;

    node_t pnode = plist->head;
    plist->head = pnode->next;
    plist->len--;

    free(pnode);
}

#include "list.h"

struct Stu
{
    int id;
    char name[20];
};

typedef struct Stu Stu;
typedef struct Stu * Stu_t;

int main()
{
    int i;
    Stu_t pstu, pstu_temp;
    list_t plist = list_create();

    for (i=0;i<10;i++)
    {
        pstu = (Stu_t)malloc(sizeof(Stu));

        if (pstu == NULL)
        {
            printf("malloc error\n");
            exit(1);
        }

        pstu->id = i;
        memcpy(pstu->name, "msg", 3);

        list_push_tail(plist, pstu);
    }

    printf("len is %d\n", plist->len);

    node_t pnode = NULL;

    for (i=0;i<11;i++)
    {
        pnode = plist->head;

        if (pnode == NULL)
        {
            printf("pnode is NULL");
            break;
        }

        pstu_temp = (Stu_t)(pnode->value);
        printf("pop head id is %d\n", pstu_temp->id);

        list_pop_head(plist);
    }

    printf("len is %d\n", plist->len);

    return 0;
}
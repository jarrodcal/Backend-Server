#include "list.h"

struct stu
{
    char name[12];
    unsigned int sex:4;
    unsigned char height;
};

typedef struct stu stu;
typedef struct stu * stu_t;

stu **getStudents(int count)
{
    stu **ppStu = (stu **)malloc(sizeof(stu *) * count);

    int i = 0;

    for (;i<count;i++)
    {
        ppStu[i] = (stu *)malloc(sizeof(stu));
        ppStu[i]->height = i+1;
    }

    return ppStu;
}

void free_stu(void *pstu)
{
    if (pstu)
    {
        free(pstu);
    }
}

int main()
{
    int i = 0;
    int count = 20;
    stu **ppStu = getStudents(count);

    list_t plist = list_create(free_stu);
    
    for (;i<20;i++)
        list_push_tail(plist, ppStu[i]);

    printf("len is %d\n", plist->len);

    i = 0;
    stu_t pstu;

    node_t pnode = NULL;

    while (i++<20)
    {
        list_pop_head(plist, pnode);
        pstu = (stu_t)(pnode->value);
        printf("%d\n", pstu->height);
    }

    return 0;
}
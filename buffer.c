#include "buffer.h"

static void buffer_expand(buffer_t bf, size_t size);
static void buffer_shrink(buffer_t bf);

buffer_t buffer_create(void* conn, size_t default_size)
{
    buffer_t bf = (buffer_t)malloc(sizeof(buffer));

    if (bf == NULL)
    {
        print_log(LOG_TYPE_ERROR, "create buff malloc is error, errno = %d", errno);
        return NULL;
    }

    bf->_default_size = default_size;
    bf->_size = bf->_default_size;
    bf->_read_index = 0;
    bf->_write_index = 0;
    bf->_conn = conn;
    bf->_data = (char*)malloc(bf->_size);

    if (bf->_data == NULL)
    {
        print_log(LOG_TYPE_ERROR, "create buff data malloc is error, errno = %d", errno);
        return NULL;
    }

    return bf;
}

void buffer_destroy(buffer_t bf)
{
    if (bf != NULL)
    {
        bf->_read_index = 0;
        bf->_write_index = 0;
        bf->_conn = NULL;
        MEM_FREE(bf->_data);
        MEM_FREE(bf);
    }
}

void buffer_reset(buffer_t bf)
{
    if (bf != NULL)
    {
        bf->_read_index = 0;
        bf->_write_index = 0;

        buffer_shrink(bf);
        bf->_size = bf->_default_size;
    }
}

char* buffer_get_write(buffer_t bf)
{
    if (bf == NULL)
    {
        print_log(LOG_TYPE_ERROR, "get write buf is error, errno = %d", errno);

        return NULL;
    }

    return bf->_data + bf->_write_index;
}

char* buffer_get_read(buffer_t bf)
{
    if (bf == NULL)
    {
        print_log(LOG_TYPE_ERROR, "get read buf is error, errno = %d", errno);

        return NULL;
    }

    return bf->_data + bf->_read_index;
}

size_t buffer_writable(buffer_t bf)
{
    if (bf == NULL)
    {
        print_log(LOG_TYPE_ERROR, "get write buf is error,  errno = %d", errno);
        return 0;
    }

    return bf->_size - bf->_write_index;
}

size_t buffer_readable(buffer_t bf)
{
    if (bf == NULL)
    {
        print_log(LOG_TYPE_ERROR, "get write buf is error, errno = %d", errno);
        return 0;
    }

    return bf->_write_index - bf->_read_index;
}

//将缓冲区读写游标放到初始位置，但缓冲区中的内容没有清空
void buffer_read(buffer_t bf, size_t size, BOOL breset)
{
    if (bf == NULL)
    {
        print_log(LOG_TYPE_ERROR, "get buffer readed is error,  errno = %d", errno);

        return;
    }

    bf->_read_index += size;

    if ((breset == TRUE) && (bf->_read_index == bf->_write_index))
    {
        bf->_read_index = 0;
        bf->_write_index = 0;
        buffer_shrink(bf);
    }
}

void buffer_write(buffer_t bf, char* pdata, size_t size)
{
    if (bf == NULL)
    {
        print_log(LOG_TYPE_ERROR, "buffer write is error, errno = %d", errno);
        return;
    }

    if (size > buffer_writable(bf))
        buffer_expand(bf, size);

    memcpy(bf->_data + bf->_write_index, pdata, size);
    bf->_write_index += size;
}

void buffer_writed(buffer_t bf, size_t size)
{
    bf->_write_index += size;   
}

//扩容
static void buffer_expand(buffer_t bf, size_t size)
{
    while (size > buffer_writable(bf))
        bf->_size *= 2;

    char* p = (char*)realloc(bf->_data, bf->_size);

    if (p == NULL)
    {
        print_log(LOG_TYPE_ERROR, "buffer expand is error, errno = %d", errno);
        return;
    }

    bf->_data = p;
}

//收缩, 快速回收申请的空间
static void buffer_shrink(buffer_t bf)
{
    char* tmp = NULL;

    if (bf == NULL)
    {
        print_log(LOG_TYPE_ERROR, "buffer shrink is error, errno = %d", errno);
        return;
    }

    if (bf->_size > bf->_default_size)
    {
        //realloc函数本身赋予了较多功能，如果申请失败会直接将原有的内存设置为NULL，这里应该采用的是先临时赋予一个变量进行判断，分配成功后再进行赋值
        tmp = realloc(bf->_data, bf->_default_size);

        if (tmp == NULL)
        {
            print_log(LOG_TYPE_ERROR, "buf realloc is error, errno = %d", errno);
            return;
        }

        bf->_data = tmp;
        bf->_size = bf->_default_size;
    }
}

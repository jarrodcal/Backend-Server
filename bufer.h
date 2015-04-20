#ifndef __BUFFER_H__
#define __BUFFER_H__

#include "common.h"
#include "log.h"

//对于网络编程，使用buffer->connector->threads的纽带关系, 可以快速的定位某个读写缓冲区对应的连接，以及该连接所在的工作线程，方便业务进行一些逻辑的快速处理
//buffer的目的是临时存储接收和发送的数据，防止读取或者发送数据不完整

struct buffer
{
    char*   _data;
    size_t  _default_size;
    size_t  _size;
    size_t  _write_index;
    size_t  _read_index;
    void*   _conn;
};

typedef struct buffer buffer;
typedef struct buffer *buffer_t;

buffer_t buffer_create(void* conn, size_t default_size);

void buffer_destroy(buffer_t bf);

void buffer_reset(buffer_t bf);

char* buffer_get_write(buffer_t bf);

char* buffer_get_read(buffer_t bf);

size_t buffer_writable(buffer_t bf);

size_t buffer_readable(buffer_t bf);

void buffer_read(buffer_t bf, size_t size, BOOL breset);

void buffer_write(buffer_t bf, char* pdata, size_t size);

void buffer_writed(buffer_t bf, size_t size);

#endif
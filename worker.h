#ifndef __SLAVE_H__
#define __SLAVE_H__

struct worker
{
    pthread_t tid;
    int epfd;
    int total_count;
    int closed_count;
};

typedef struct worker worker;
typedef struct worker * worker_t;

worker_t worker_create();
void worker_close(master_t pmaster);
void worker_loop(master_t pmaster);

void master_add_fd(master_t pmaster, int fd, int op);
void fs_accept(master_t pmaster);
void channel_handle_read(master_t pmaster, int sockfd);
void channel_handle_write(master_t pmaster, int sockfd, char *buf);

#endif
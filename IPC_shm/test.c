#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <assert.h>

#include "shm_queue.h"

#define SHM_NAME "/ipc"
#define CAPACITY 1000
#define MESSAGES_N 10000000

void print_time(const char *msg);

int run_parent(void);
int run_child(void);

int main(void)
{
    int ret = 0;

    switch (fork())
    {
    case -1:
        perror("fork");
        ret = 1;
        break;

    case 0:
        ret = run_parent();

        wait(NULL);
        break;

    default:
        ret = run_child();
        break;
    }

    return ret;
}

int run_parent(void)
{
    shm_queue_t *shm_queue = NULL;

    if (shm_queue_create(sizeof(uint32_t), CAPACITY, SHM_NAME, &shm_queue) != 0)
    {
        perror("shm_queue_create_call");
        return 1;
    }

    print_time("start");

    for (uint32_t i = 0; i < MESSAGES_N; i++)
        if (shm_queue_push(shm_queue, (void *)&i) != 0)
        {
            perror("shm_queue_push_call");
            break;
        }

    if (shm_queue_close(shm_queue) != 0)
    {
        perror("shm_queue_close");
        return 1;
    }

    return 0;
}

int run_child(void)
{
    shm_queue_t *shm_queue = NULL;
    if (shm_queue_open(SHM_NAME, &shm_queue) != 0)
    {
        perror("shm_queue_open_call\n");
        return 1;
    }

    uint32_t data = 0;
    for (uint32_t i = 0; i < MESSAGES_N; i++)
    {
        if (shm_queue_pop(shm_queue, &data) != 0)
        {
            perror("shm_queue_pop_call");
            break;
        }

        // printf("%u: GOT DATA %u\n", i, data);
    }

    print_time("finish");

    if (shm_queue_close(shm_queue) != 0)
    {
        perror("shm_queue_close_call");
        return 1;
    }

    return 0;
}

void print_time(const char *msg)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    printf("%sed at \t%lu us\n", msg, ((time.tv_sec) * 1000000 + time.tv_usec) % 10000000);
}
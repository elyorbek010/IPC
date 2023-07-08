#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <assert.h>

#include "shm_queue.h"

#define SHM_NAME "/ipc_14"
#define CAPACITY 1000
#define MESSAGES_N 10000000

void print_time(const char *msg)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    printf("%sed at %lu us\n", msg, ((time.tv_sec) * 1000000 + time.tv_usec) % 10000000);
}

int main(void)
{
    struct timeval stop, start;
    int err_code;
    shm_queue_t *shm_queue = NULL;

    pid_t pid = fork();

    switch (pid)
    {
    case -1: // error
        perror("fork");
        break;

    case 0:
    { // parent
        assert(shm_queue_create(sizeof(uint32_t), CAPACITY, SHM_NAME, &shm_queue) == 0);

        print_time("start");

        for (uint32_t i = 0; i < MESSAGES_N; i++)
            assert(shm_queue_push(shm_queue, (void *)&i) == 0);

        assert(shm_queue_close(shm_queue) == 0);

        wait(NULL);
        break;
    }

    default:
    { // child
        if (shm_queue_open(SHM_NAME, &shm_queue))
        {
            printf("couldn't open queue\n");
            return 0;
        }

        char buf[sizeof(uint32_t)];
        for (uint32_t i = 0; i < MESSAGES_N; i++)
            assert(shm_queue_pop(shm_queue, buf) == 0);

        print_time("finish");

        assert(shm_queue_close(shm_queue) == 0);
        break;
    }
    }

    return 0;
}
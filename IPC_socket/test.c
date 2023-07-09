#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <sys/wait.h>

#include "socket_pipe.h"

#define SOCKET_FILE_NAME "/tmp/somepath"

#define MESSAGE_N 10000000

void print_time(const char *msg)
{
    struct timeval time;
    gettimeofday(&time, NULL);
    printf("%sed at \t%lu us\n", msg, ((time.tv_sec) * 1000000 + time.tv_usec) % 10000000);
}

int run_server(void);
int run_client(void);

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
        ret = run_server();
        
        wait(NULL);
        break;

    default:
        ret = run_client();
        break;
    }

    return ret;
}

int run_server(void)
{
    socket_pipe_t *socket_pipe = NULL;

    if (socket_pipe_create(sizeof(uint32_t), SOCKET_FILE_NAME, &socket_pipe) != 0)
    {
        perror("socket_create_call");
        return 1;
    }

    print_time("start");

    for (uint32_t i = 0; i < MESSAGE_N; i++)
        if ((socket_pipe_send(socket_pipe, (void *)&i)) != 0)
        {
            perror("socket_pipe_send_call");
            break;
        }

    if (socket_pipe_destroy(socket_pipe) != 0)
    {
        perror("socket_pipe_destroy_call");
        return 1;
    }

    return 0;
}

int run_client(void)
{
    socket_pipe_t *socket_pipe = NULL;

    if (socket_pipe_open(SOCKET_FILE_NAME, &socket_pipe) != 0)
    {
        perror("socket_pipe_open_call");
        return 1;
    }

    uint32_t data = 0;
    for (uint32_t i = 0; i < MESSAGE_N; i++)
    {
        if (socket_pipe_recv(socket_pipe, (void *)&data) != 0)
        {
            perror("socket_pipe_recv_call");
            break;
        }

        // printf("%u: GOT DATA %u\n", i, data);
    }

    print_time("end");

    if (socket_pipe_close(socket_pipe) != 0)
    {
        perror("socket_pipe_close_call");
        return 1;
    }
}
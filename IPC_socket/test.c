#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <stdint.h>
#include <sys/wait.h>

#include "socket_pipe.h"

#define SOCKET_FILE_NAME "/tmp/somepath10"

#define MESSAGE_N 100

int main(void)
{
    pid_t pid = fork();

    socket_pipe_t *socket_pipe = NULL;

    switch (pid)
    {
    case -1:
        perror("fork");
        break;

    case 0: // parent
    {
        assert(socket_pipe_create(sizeof(uint32_t), SOCKET_FILE_NAME, &socket_pipe) == 0);

        for (uint32_t i = 0; i < MESSAGE_N; i++)
            assert(socket_pipe_send(socket_pipe, (void *)&i) == 0);

        assert(socket_pipe_destroy(socket_pipe) == 0);
        wait(NULL);
    }
    default: // child
    {
        uint32_t data = 0;
        assert(socket_pipe_open(SOCKET_FILE_NAME, &socket_pipe) == 0);
        
        for (uint32_t i = 0; i < MESSAGE_N; i++)
        {
            assert(socket_pipe_recv(socket_pipe, (void *)&data) == 0);
            printf("%u: GOT DATA %u\n", i, data);
        }

        assert(socket_pipe_close(socket_pipe) == 0);
    }
    }

    return 0;
}
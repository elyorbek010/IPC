#include "stdint.h"
#include "error_handler.h"
#include "benchmark_helper.h"

extern "C"
{
#include "socket_pipe.h"
}

#define FILE_NAME "/tmp/somepath"

int benchmark_setup(int64_t size, int is_server, void **p_ipc)
{
    if (is_server == 0)
        RETURN_ON_ERROR(socket_pipe_open(FILE_NAME, (socket_pipe_t **)p_ipc));
    else
        RETURN_ON_ERROR(socket_pipe_create(sizeof(uint32_t), FILE_NAME, (socket_pipe_t **)p_ipc));

    return 0;
}

int benchmark_teardown(int is_server, void *p_ipc)
{
    if (is_server == 0)
        EXIT_ON_ERROR(socket_pipe_close((socket_pipe_t *)p_ipc));
    else
        EXIT_ON_ERROR(socket_pipe_destroy((socket_pipe_t *)p_ipc));

    return 0;
}

int pop_many_elements(size_t n, void *p_ipc)
{
    uint32_t data = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        RETURN_ON_ERROR(socket_pipe_recv((socket_pipe_t *)p_ipc, (void *)&data));

        // printf("%u: GOT DATA %u\n", i, data);
    }

    return 0;
}

int push_many_elements(size_t n, void *p_ipc)
{
    for (uint32_t i = 0; i < n; i++)
        RETURN_ON_ERROR(socket_pipe_send((socket_pipe_t *)p_ipc, (void *)&i));

    return 0;
}
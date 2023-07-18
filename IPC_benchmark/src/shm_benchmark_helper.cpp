#include "stdint.h"
#include "error_handler.h"
#include "benchmark_helper.h"

extern "C"
{
#include "shm_queue.h"
}

#define FILE_NAME "/IPC_SHM"

int benchmark_setup(int64_t size, int is_server, void **p_ipc)
{
    if (is_server == 0)
        RETURN_ON_ERROR(shm_queue_open(FILE_NAME, (shm_queue_t **)p_ipc));
    else
        RETURN_ON_ERROR(shm_queue_create(sizeof(uint32_t), size, FILE_NAME, (shm_queue_t **)p_ipc));

    return 0;
}

int benchmark_teardown(int is_server, void *p_ipc)
{
    if (is_server == 0)
        EXIT_ON_ERROR(shm_queue_close((shm_queue_t *)p_ipc));
    else
        EXIT_ON_ERROR(shm_queue_close((shm_queue_t *)p_ipc));

    return 0;
}

int pop_many_elements(size_t n, void *p_ipc)
{
    uint32_t data = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        RETURN_ON_ERROR(shm_queue_pop((shm_queue_t *)p_ipc, &data));

        // printf("%u: GOT DATA %u\n", i, data);
    }

    return 0;
}

int push_many_elements(size_t n, void *p_ipc)
{
    for (uint32_t i = 0; i < n; i++)
        RETURN_ON_ERROR(shm_queue_push((shm_queue_t *)p_ipc, &i));

    return 0;
}
extern "C"
{
#include "../shm_queue.h"
}

#include <benchmark/benchmark.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/eventfd.h>

#define EXIT_ON_ERROR(val) \
    do                     \
    {                      \
        if (val != 0)      \
        {                  \
            perror(#val);  \
            exit(1);       \
        }                  \
    } while (0)

#define RETURN_ON_ERROR(val) \
    do                       \
    {                        \
        if (val != 0)        \
        {                    \
            perror(#val);    \
            return -1;       \
        }                    \
    } while (0)

#define SHM_NAME "/IPC_SHM"

static int pop_many_elements(shm_queue_t *shm_queue, size_t n);
static int push_many_elements(shm_queue_t *shm_queue, size_t n);

static void Bench_shm_simulate(benchmark::State &state);

BENCHMARK(Bench_shm_simulate)->RangeMultiplier(2)->Range(1, 10000000);

BENCHMARK_MAIN();

static void Bench_shm_simulate(benchmark::State &state)
{
    // set up
    eventfd_t eventfd_val = 10;
    int start_fd, end_fd;
    pid_t pid = -1;
    shm_queue_t *shm_queue = NULL;

    start_fd = eventfd(0, 0);
    end_fd = eventfd(0, 0);

    if (start_fd < 0 || end_fd < 0)
        exit(1);

    pid = fork();

    if (pid == 0)
    {
        EXIT_ON_ERROR(shm_queue_open(SHM_NAME, &shm_queue));
    }
    else if (pid > 0)
    {
        EXIT_ON_ERROR(shm_queue_create(sizeof(uint32_t), state.range(0), SHM_NAME, &shm_queue));
    }

    // main loop
    for (auto _ : state)
    {
        switch (pid)
        {
        case -1:
            perror("fork");
            exit(1);
            break;

        case 0:
        {
            eventfd_read(start_fd, &eventfd_val);

            EXIT_ON_ERROR(pop_many_elements(shm_queue, state.range(0)));

            eventfd_write(end_fd, eventfd_val);
            break;
        }
        default:
        {
            eventfd_write(start_fd, eventfd_val);

            EXIT_ON_ERROR(push_many_elements(shm_queue, state.range(0)));

            eventfd_read(end_fd, &eventfd_val);
            break;
        }
        }
    }

    // tear down
    EXIT_ON_ERROR(shm_queue_close(shm_queue)); // last referencing process, so, deletes queue

    if (pid == 0)
        exit(0);
    else
        wait(NULL);
}

static int pop_many_elements(shm_queue_t *shm_queue, size_t n)
{
    uint32_t data = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        RETURN_ON_ERROR(shm_queue_pop(shm_queue, &data));

        // printf("%u: GOT DATA %u\n", i, data);
    }

    return 0;
}

static int push_many_elements(shm_queue_t *shm_queue, size_t n)
{
    for (uint32_t i = 0; i < n; i++)
        RETURN_ON_ERROR(shm_queue_push(shm_queue, &i));

    return 0;
}
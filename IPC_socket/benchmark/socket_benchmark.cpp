extern "C"
{
#include "../socket_pipe.h"
}

#include <benchmark/benchmark.h>
#include <unistd.h>
#include <sys/wait.h>
#include <thread>
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

#define SOCKET_FILE_NAME "/tmp/somepath"

static int pop_many_elements(socket_pipe_t *socket_pipe, size_t n);
static int push_many_elements(socket_pipe_t *socket_pipe, size_t n);

static void Bench_socket_simulate(benchmark::State &state);

BENCHMARK(Bench_socket_simulate)->RangeMultiplier(2)->Range(1, 10000000);

BENCHMARK_MAIN();

static void Bench_socket_simulate(benchmark::State &state)
{
    // set up
    eventfd_t eventfd_val = 10;
    int start_fd, end_fd;
    pid_t pid = -1;
    socket_pipe_t *socket_pipe_client;
    socket_pipe_t *socket_pipe_server;
    auto thread_id = std::thread([&]()
                                 { EXIT_ON_ERROR(socket_pipe_open(SOCKET_FILE_NAME, &socket_pipe_client)); });

    EXIT_ON_ERROR(socket_pipe_create(sizeof(uint32_t), SOCKET_FILE_NAME, &socket_pipe_server));

    thread_id.join();

    start_fd = eventfd(0, 0);
    end_fd = eventfd(0, 0);

    if (start_fd < 0 || end_fd < 0)
        exit(1);

    pid = fork();

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

            EXIT_ON_ERROR(pop_many_elements(socket_pipe_client, state.range(0)));

            eventfd_write(end_fd, eventfd_val);
            break;
        }
        default:
        {
            eventfd_write(start_fd, eventfd_val);

            EXIT_ON_ERROR(push_many_elements(socket_pipe_server, state.range(0)));

            eventfd_read(end_fd, &eventfd_val);
            break;
        }
        }
    }

    // tear down
    if (pid == 0)
    {
        EXIT_ON_ERROR(socket_pipe_close(socket_pipe_client));
        exit(0);
    }
    else
    {
        EXIT_ON_ERROR(socket_pipe_destroy(socket_pipe_server));
        wait(NULL);
    }
}

static int pop_many_elements(socket_pipe_t *socket_pipe, size_t n)
{
    uint32_t data = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        RETURN_ON_ERROR(socket_pipe_recv(socket_pipe, (void *)&data));

        // printf("%u: GOT DATA %u\n", i, data);
    }

    return 0;
}

static int pop_many_elements(socket_pipe_t *socket_pipe, size_t n)
{
    uint32_t data = 0;
    for (uint32_t i = 0; i < n; i++)
    {
        RETURN_ON_ERROR(socket_pipe_recv(socket_pipe, (void *)&data));

        // printf("%u: GOT DATA %u\n", i, data);
    }

    return 0;
}

static int push_many_elements(socket_pipe_t *socket_pipe, size_t n)
{
    for (uint32_t i = 0; i < n; i++)
        RETURN_ON_ERROR(socket_pipe_send(socket_pipe, (void *)&i));

    return 0;
}
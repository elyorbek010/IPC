#include <benchmark/benchmark.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/eventfd.h>

#include "includes/error_handler.h"

#include "includes/benchmark_helper.h"

static void Bench_simulate(benchmark::State &state);

BENCHMARK(Bench_simulate)->RangeMultiplier(2)->Range(1, 10000000);

BENCHMARK_MAIN();

static void Bench_simulate(benchmark::State &state)
{
    // set up
    void *p_ipc = NULL;
    pid_t pid = -1;

    eventfd_t eventfd_val = 1;
    int start_fd = eventfd(0, 0);
    int end_fd = eventfd(0, 0);

    if (start_fd < 0 || end_fd < 0)
        exit(1);

    if ((pid = fork()) < 0)
        exit(1);

    EXIT_ON_ERROR(benchmark_setup(state.range(0), pid, &p_ipc));

    // main loop
    for (auto _ : state)
    {
        switch (pid)
        {
        case 0:
        {
            eventfd_read(start_fd, &eventfd_val);

            EXIT_ON_ERROR(pop_many_elements(state.range(0), p_ipc));

            eventfd_write(end_fd, eventfd_val);
            break;
        }
        default:
        {
            eventfd_write(start_fd, eventfd_val);

            EXIT_ON_ERROR(push_many_elements(state.range(0), p_ipc));

            eventfd_read(end_fd, &eventfd_val);
            break;
        }
        }
    }

    // tear down
    EXIT_ON_ERROR(benchmark_teardown(pid, p_ipc));

    close(start_fd);
    close(end_fd);

    if (pid == 0)
    {
        exit(0);
    }
    else
    {
        wait(NULL);
    }
}
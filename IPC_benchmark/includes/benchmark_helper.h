#ifndef BENCHMARK_HELPER
#define BENCHMARK_HELPER

#include <stdint.h>
#include <stddef.h>

int benchmark_setup(int64_t size, int is_server, void **p_ipc);

int benchmark_teardown(int is_server, void *p_ipc);

int pop_many_elements(size_t n, void *p_ipc);

int push_many_elements(size_t n, void *p_ipc);

#endif // BENCHMARK_HELPER
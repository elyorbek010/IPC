cmake_minimum_required(VERSION 3.14)

set(This ShmBenchmark)

set(Sources 
    shm_benchmark.cpp
)

add_executable(${This} ${Sources})

target_link_libraries(${This} PUBLIC
    IPC_shm
    benchmark::benchmark
)
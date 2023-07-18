cmake_minimum_required(VERSION 3.16.3)

set(This IPC_benchmark)

add_executable(shm_benchmark benchmark.cpp src/shm_benchmark_helper.cpp)
add_executable(socket_benchmark benchmark.cpp src/socket_benchmark_helper.cpp)

target_link_libraries(shm_benchmark PUBLIC
    IPC_shm
    benchmark::benchmark
)

target_link_libraries(socket_benchmark PUBLIC
    IPC_socket
    benchmark::benchmark
)

target_include_directories(shm_benchmark PRIVATE includes)
target_include_directories(socket_benchmark PRIVATE includes)
cmake_minimum_required(VERSION 3.14)

set(This SocketBenchmark)

set(Sources 
    socket_benchmark.cpp
)

add_executable(${This} ${Sources})

target_link_libraries(${This} PUBLIC
    IPC_socket
    benchmark::benchmark
)
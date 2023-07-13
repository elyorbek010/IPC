cmake_minimum_required(VERSION 3.14)

set(This IPC_socket)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENTEND_CODE ON)

set(Headers 
    socket_pipe.h
)

set(Sources
    socket_pipe.c
)

add_library(${This} STATIC ${Sources} ${Headers})

add_subdirectory(benchmark)
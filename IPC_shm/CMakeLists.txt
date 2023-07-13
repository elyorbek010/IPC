cmake_minimum_required(VERSION 3.14)

set(This IPC_shm)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_POSITION_INDEPENTEND_CODE ON)

set(Headers 
    shm_queue.h
)

set(Sources
    shm_queue.c
)

add_library(${This} STATIC ${Sources} ${Headers})

add_subdirectory(benchmark)
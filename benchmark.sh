#! /usr/bin/bash
mkdir build
cmake -B build
cmake --build build
tools/compare.py benchmarks build/IPC_benchmark/shm_benchmark build/IPC_benchmark/socket_benchmark

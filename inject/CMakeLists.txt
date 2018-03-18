cmake_minimum_required(VERSION 3.9)
project(inject C)

set(CMAKE_C_STANDARD 99)

add_library(inject linux-inject/utils.c linux-inject/ptrace.c inject.c inject.h)
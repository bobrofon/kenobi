cmake_minimum_required(VERSION 3.9)
project(inject C)

set(CMAKE_C_STANDARD 99)

add_library(inject utils.c utils.h ptrace.c ptrace.h inject.c inject.h)

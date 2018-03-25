cmake_minimum_required(VERSION 3.9)
project(hello_there C)

set(CMAKE_C_STANDARD 11)

add_library(hello_there SHARED library.c)
include(enable_warnings)

target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})
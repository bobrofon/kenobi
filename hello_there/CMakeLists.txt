cmake_minimum_required(VERSION 3.9)
project(hello_there)

set(CMAKE_CXX_STANDARD 17)

add_library(hello_there SHARED library.cpp)
target_link_libraries(${PROJECT_NAME} -lstdc++fs ${CMAKE_DL_LIBS})
cmake_minimum_required(VERSION 3.9)
project(kenobi)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(hello_there)
add_subdirectory(inject)

add_executable(kenobi main.cpp dark_side.cpp dark_side.h)

target_link_libraries(${PROJECT_NAME} -lstdc++fs ${CMAKE_DL_LIBS} inject)

add_dependencies(${PROJECT_NAME} hello_there)

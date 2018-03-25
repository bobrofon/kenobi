cmake_minimum_required(VERSION 3.9)
project(kenobi)

set(CMAKE_CXX_STANDARD 17)

add_executable(kenobi main.cpp dark_side.cpp dark_side.h)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(enable_warnings)

find_package( Threads )

add_subdirectory(hello_there)
add_subdirectory(inject)

target_link_libraries(${PROJECT_NAME} -lstdc++fs inject ${CMAKE_THREAD_LIBS_INIT})

add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/libhello_there.so
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/hello_there/libhello_there.so
        COMMAND
            ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/hello_there/libhello_there.so ${CMAKE_CURRENT_BINARY_DIR}/libhello_there.so
        COMMENT
            "copy libhello_there.so"
        VERBATIM
)

add_custom_target(
        copy_hello_lib
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/libhello_there.so
)

add_dependencies(copy_hello_lib hello_there)
add_dependencies(${PROJECT_NAME} copy_hello_lib)

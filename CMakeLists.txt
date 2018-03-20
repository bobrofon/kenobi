cmake_minimum_required(VERSION 3.9)
project(kenobi)

set(CMAKE_CXX_STANDARD 17)

find_package( Threads )

add_subdirectory(hello_there)
add_subdirectory(inject)

add_executable(kenobi main.cpp dark_side.cpp dark_side.h)

target_link_libraries(${PROJECT_NAME} -lstdc++fs ${CMAKE_DL_LIBS} inject ${CMAKE_THREAD_LIBS_INIT})

add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/libhello_there.so
        DEPENDS
            ${CMAKE_CURRENT_BINARY_DIR}/hello_there/libhello_there.so
        COMMAND
            cp ${CMAKE_CURRENT_BINARY_DIR}/hello_there/libhello_there.so ${CMAKE_CURRENT_BINARY_DIR}/libhello_there.so
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

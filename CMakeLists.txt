cmake_minimum_required(VERSION 3.9)
project(kenobi)

set(CMAKE_CXX_STANDARD 17)

find_package( Threads )

add_subdirectory(hello_there)
add_subdirectory(inject)

add_executable(kenobi main.cpp dark_side.cpp dark_side.h)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    target_compile_options(${PROJECT_NAME} PRIVATE
            -Weverything
            -Wno-c++98-compat
            -Wno-c++98-compat-pedantic
            -Wno-padded
            -Wno-shadow
            -Wno-weak-vtables
            -pedantic
            -pedantic-errors)
else ()
    set_target_properties(${PROJECT_NAME} PROPERTIES
            COMPILE_FLAGS "-Wall"
            COMPILE_FLAGS "-Wextra"
            COMPILE_FLAGS "-Waddress"
            COMPILE_FLAGS "-Warray-bounds"
            COMPILE_FLAGS "-Wbuiltin-macro-redefined"
            COMPILE_FLAGS "-Wconversion"
            COMPILE_FLAGS "-Wctor-dtor-privacy"
            COMPILE_FLAGS "-Winit-self"
            COMPILE_FLAGS "-Wnon-virtual-dtor"
            COMPILE_FLAGS "-Wold-style-cast"
            COMPILE_FLAGS "-Woverloaded-virtual"
            COMPILE_FLAGS "-Wsuggest-attribute=const"
            COMPILE_FLAGS "-Wsuggest-attribute=noreturn"
            COMPILE_FLAGS "-Wsuggest-attribute=pure"
            COMPILE_FLAGS "-Wswitch"
            COMPILE_FLAGS "-Wunreachable-code"
            COMPILE_FLAGS "-pedantic"
            COMPILE_FLAGS "-pedantic-errors")
endif ()

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

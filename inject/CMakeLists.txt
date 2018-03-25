cmake_minimum_required(VERSION 3.9)
project(inject C)

set(CMAKE_C_STANDARD 99)

add_library(inject utils.c utils.h ptrace.c ptrace.h inject.c inject.h)

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

target_link_libraries(${PROJECT_NAME} ${CMAKE_DL_LIBS})

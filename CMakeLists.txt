cmake_minimum_required(VERSION 3.23)
project(common_modules)

set(CMAKE_CXX_STANDARD 20)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

include_directories(../modules)

add_subdirectory(Google_Test)

add_subdirectory(modules/logger)
add_subdirectory(modules/msg)
add_subdirectory(modules/config)
add_subdirectory(modules/plan)
add_subdirectory(modules/tools)
add_subdirectory(modules/api)
add_subdirectory(modules/data-waiter)
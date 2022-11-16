cmake_minimum_required(VERSION 3.23)
project(common_modules)
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

include_directories(..)

add_subdirectory(logger)
add_subdirectory(msg)
add_subdirectory(config)
add_subdirectory(plan)
add_subdirectory(tools)
add_subdirectory(api)
add_subdirectory(data-waiter)

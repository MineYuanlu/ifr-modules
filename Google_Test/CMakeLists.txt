# 'Google_test' is the subproject name
project(Google_tests)

# 'lib' is the folder with Google Test sources
add_subdirectory(lib)
MESSAGE("GTest Dir ${gtest_SOURCE_DIR}")
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

include_directories(../modules)
include_directories(../lib)


aux_source_directory(./testers SRCX)
add_executable(Google_Tests_run ${SRCX})
target_link_libraries(Google_Tests_run gtest gtest_main plan config tools api behavior)

cmake_minimum_required(VERSION 3.16)

if(NOT BUILD_TESTING)
    return()
endif()

enable_testing()

find_package(GTest REQUIRED)

add_definitions(-DTEST_DIR="${CMAKE_CURRENT_BINARY_DIR}/tests_output")

add_subdirectory(logger_test)
add_subdirectory(general_test)
add_subdirectory(common_test)


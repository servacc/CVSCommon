cmake_minimum_required(VERSION 3.16)

include(CTest)

if(NOT BUILD_TESTING)
    return()
endif()

project(config_test)

enable_testing()
find_package(GTest REQUIRED)

add_executable(${PROJECT_NAME} configtest.cpp)
target_link_libraries(${PROJECT_NAME} CVSCommon GTest::gtest GTest::gtest_main)

add_test(AllTestsInFoo foo)

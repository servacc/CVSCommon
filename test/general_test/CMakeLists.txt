cmake_minimum_required(VERSION 3.16)

project(general_test)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
  PRIVATE
        general_test.cpp)

target_link_libraries(${PROJECT_NAME} cvs::Common GTest::gtest_main)

add_test(GeneralTests ${PROJECT_NAME})

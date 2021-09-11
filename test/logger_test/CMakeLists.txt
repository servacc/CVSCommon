cmake_minimum_required(VERSION 3.16)

project(logger_test)

add_executable(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
  PRIVATE
    opencv_test.cpp
    opencv_cfg_test.cpp
    factory_test.cpp
    main.cpp)

target_link_libraries(${PROJECT_NAME} cvs::Logger_opencv GTest::gtest GTest::gtest_main)


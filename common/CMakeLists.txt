cmake_minimum_required(VERSION 3.17)

project(Common)

add_library(${PROJECT_NAME} STATIC)

cvs_add_to_alias(${PROJECT_NAME})
cvs_library_version(TARGET ${PROJECT_NAME}
  VERSION_MAJOR ${CVSCommon_VERSION_MAJOR}
  VERSION_MINOR ${CVSCommon_VERSION_MINOR}
  )

cvs_create_version_file(NAME ${PROJECT_NAME}
                        MAJOR ${CVSCommon_VERSION_MAJOR}
                        MINOR ${CVSCommon_VERSION_MINOR}
                        PATCH ${CVSCommon_VERSION_PATCH}
                        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generate/cvs/common")


generate_export_header(${PROJECT_NAME}
  BASE_NAME CVSCommon
  EXPORT_FILE_NAME ${CMAKE_CURRENT_BINARY_DIR}/export/cvs/common/export.hpp)

set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN 1)

target_sources(${PROJECT_NAME}
  PRIVATE
    include/cvs/common/constexprString.hpp
    include/cvs/common/general.hpp
    include/cvs/common/config.hpp
    include/cvs/common/staticFactory.hpp
    include/cvs/common/factory.hpp

    src/general.cpp
    src/config.cpp)


target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generate>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/CVSCommon>)

target_link_libraries(${PROJECT_NAME}
  Boost::boost fmt::fmt)

cvs_install(NAME CVSCommon
  VERSION ${CVSCommon_VERSION}
  TARGETS_DEV ${PROJECT_NAME}
  FILES_DEV
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CVSCommon/cvs/common
    FILES include/cvs/common/constexprString.hpp
          include/cvs/common/general.hpp
          include/cvs/common/config.hpp
          include/cvs/common/staticFactory.hpp
          include/cvs/common/factory.hpp
          ${CMAKE_CURRENT_BINARY_DIR}/export/cvs/common/export.hpp
          ${CMAKE_CURRENT_BINARY_DIR}/generate/cvs/common/version.hpp)

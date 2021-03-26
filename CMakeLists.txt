cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(CVSCommon)

project(CVSCommon CVS_PROJECT
                  VERSION 0.1.0
                  DESCRIPTION "CVS common library"
                  LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Boost REQUIRED)

add_library(${PROJECT_NAME} STATIC)

cvs_add_to_alias(${PROJECT_NAME})
cvs_library_version(${PROJECT_NAME})

cvs_create_version_file(NAME ${PROJECT_NAME}
                        MAJOR ${PROJECT_VERSION_MAJOR}
                        MINOR ${PROJECT_VERSION_MINOR}
                        PATCH ${PROJECT_VERSION_PATCH}
                        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generate/cvs/common")

target_sources(${PROJECT_NAME}
  PRIVATE
    include/cvs/common/configbase.hpp
    include/cvs/common/config.hpp
    include/cvs/common/configutils.hpp
    include/cvs/common/factory.hpp

    src/config.cpp)


target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generate>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>)

target_link_libraries(${PROJECT_NAME}
  Boost::boost)

set(CVSCOMMON_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmakemodules)

add_subdirectory(test)

include(cmake/CVSInstall.cmake)

cvs_install(TARGETS_DEV ${PROJECT_NAME}
  FILES_DEV
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/cvs/common
    FILES include/cvs/common/configbase.hpp
          include/cvs/common/config.hpp
          include/cvs/common/configutils.hpp
          include/cvs/common/factory.hpp
          ${CMAKE_CURRENT_BINARY_DIR}/generate/cvs/common/version.hpp)

cvs_install(NAME ${PROJECT_NAME}CMakeHelpers
  CONFIG install/CVSCommonCMakeHelpersConfig.cmake.in
  FILES_DEV
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmakemodules
    FILES cmake/CVSGitHelpers.cmake
          cmake/CVSInstall.cmake
          cmake/CVSCommon.cmake
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmakemodules/templates
    FILES cmake/templates/version.hpp.in)

cvs_package()

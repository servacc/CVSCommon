cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

include(CVSCommon)

project(CVSCommon CVS_PROJECT
                  VERSION 0.2.0
                  DESCRIPTION "CVS common library"
                  LANGUAGES CXX)

include(GNUInstallDirs)
include(GenerateExportHeader)
include(CTest)

include(cmake/CVSInstall.cmake)

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Boost REQUIRED)
find_package(fmt REQUIRED)

add_subdirectory(common)
add_subdirectory(logger)

cvs_create_version_file(NAME ${PROJECT_NAME}
                        MAJOR ${PROJECT_VERSION_MAJOR}
                        MINOR ${PROJECT_VERSION_MINOR}
                        PATCH ${PROJECT_VERSION_PATCH}
                        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/generate/cvs/common")

add_subdirectory(test)

set(CVSCOMMON_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmakemodules)

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

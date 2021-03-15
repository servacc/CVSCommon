cmake_minimum_required(VERSION 3.16)

include(GNUInstallDirs)
include(cmake/CVSCommon.cmake)

project(CVSCommon
  VERSION 0.1.0
  DESCRIPTION "CVS common library (${GIT_DESCRIPTOR})"
  HOMEPAGE_URL ${CVS_HOMEPAGE_URL})

add_library(${PROJECT_NAME} INTERFACE)

cvs_library_version()

target_include_directories(${PROJECT_NAME}
  INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}>)

set(CVSCOMMON_MODULE_PATH ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmakemodules)

include(cmake/CVSInstall.cmake)

cvs_install(TARGETS_DEV ${PROJECT_NAME}
  FILES_DEV
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/cvs/common
    FILES include/cvs/common/factory.hpp
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmakemodules
    FILES cmake/CVSBuildNumber.cmake
          cmake/CVSGitHelpers.cmake
          cmake/CVSInstall.cmake
          cmake/CVSCommon.cmake)

cvs_package()

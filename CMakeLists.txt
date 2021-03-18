cmake_minimum_required(VERSION 3.16)

include(GNUInstallDirs)
include(cmake/CVSCommon.cmake)

project(CVSCommon
  VERSION 0.1.0
  DESCRIPTION "CVS common library (${GIT_DESCRIPTOR})"
  HOMEPAGE_URL ${CVS_HOMEPAGE_URL})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

message(NOTICE "You should use Boost_DIR if you need to use the CVSCommon package before declaring a project.")
find_package(Boost 1.70 REQUIRED)

add_library(${PROJECT_NAME} STATIC)
add_library(cvs ALIAS ${PROJECT_NAME})

cvs_library_version()

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
    DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}/cmakemodules
    FILES cmake/CVSBuildNumber.cmake
          cmake/CVSGitHelpers.cmake
          cmake/CVSInstall.cmake
          cmake/CVSCommon.cmake)

cvs_package()

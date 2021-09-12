cmake_minimum_required(VERSION 3.16)

project(Logger)

string(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWER)

option(CVSLOGGER_SHARED "" ON)
option(CVSLOGGER_OPENCV_IMG "" ON)

option(CVSLOGGER_ENABLE_STD_BY_DEFAULT "Enable stdout sink by default" OFF)
option(CVSLOGGER_ENABLE_SYSD_BY_DEFAULT "Enable systemd sink by default" ON)

include(GenerateExportHeader)

if (NOT SPDLOG_FMT_EXTERNAL OR NOT SPDLOG_FMT_EXTERNAL_HO)
  message(NOTICE "Set SPDLOG_FMT_EXTERNAL to ON. "
    "FPSLogger is not compatible with internal fmt in spdlog version 1.8.2 and older.")
  set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "")
endif()

find_package(OpenCV REQUIRED)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(systemd REQUIRED IMPORTED_TARGET libsystemd)

if(CVSLOGGER_SHARED)
  add_library(${PROJECT_NAME} SHARED)
else()
  add_library(${PROJECT_NAME} STATIC)
endif()

cvs_add_to_alias(${PROJECT_NAME})
cvs_library_version(TARGET ${PROJECT_NAME}
  VERSION_MAJOR ${CVSCommon_VERSION_MAJOR}
  VERSION_MINOR ${CVSCommon_VERSION_MINOR})

generate_export_header(${PROJECT_NAME}
  BASE_NAME CVSLogger
  EXPORT_FILE_NAME
    ${CMAKE_CURRENT_BINARY_DIR}/export/cvs/logger/export.hpp)

target_sources(${PROJECT_NAME}
  PRIVATE
    include/cvs/logger/logging.hpp
    include/cvs/logger/loggable.hpp
    include/cvs/logger/tools/fpsLogger.hpp
    include/cvs/logger/argumentPreprocessor.hpp
    include/cvs/logger/loggerTypes.hpp

    include/cvs/logger/opencvHelper.hpp

    src/tools/fpsLogger.cpp
    src/logging.cpp)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    spdlog::spdlog
    PkgConfig::systemd
    cvs::Common
    Boost::boost)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generate>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/CVSCommon>)

target_compile_definitions(${PROJECT_NAME}
  PUBLIC
    SPDLOG_FMT_EXTERNAL
    $<$<BOOL:${CVSLOGGER_ENABLE_STD_BY_DEFAULT}>:CVSLOGGER_STD_ENABLED>
    $<$<BOOL:${CVSLOGGER_ENABLE_SYSD_BY_DEFAULT}>:CVSLOGGER_SYSD_ENABLED>)

add_library(${PROJECT_NAME}OpenCV INTERFACE)
add_library(cvs::${PROJECT_NAME}OpenCV ALIAS ${PROJECT_NAME}OpenCV)

target_link_libraries(${PROJECT_NAME}OpenCV
  INTERFACE
    cvs::Logger
    opencv_imgcodecs opencv_core)

target_compile_definitions(${PROJECT_NAME}OpenCV
  INTERFACE
    CVS_LOGGER_OPENCV_ENABLED)


cvs_install(NAME CVSLogger
  VERSION ${CVSCommon_VERSION}
  TARGETS_BIN ${PROJECT_NAME}
  FILES_DEV
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CVSCommon/cvs/logger
    FILES include/cvs/logger/logging.hpp
          include/cvs/logger/loggable.hpp
          include/cvs/logger/argumentPreprocessor.hpp
          include/cvs/logger/loggerTypes.hpp
          ${CMAKE_CURRENT_BINARY_DIR}/export/cvs/logger/export.hpp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CVSCommon/cvs/logger/tools
    FILES include/cvs/logger/tools/fpsLogger.hpp)


cvs_install(NAME CVSLoggerOpenCV
  VERSION ${CVSCommon_VERSION}
  TARGETS_DEV ${PROJECT_NAME}OpenCV
  COMPONENT_DEV cvhelper
  FILES_DEV
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/CVSCommon/cvs/logger
    FILES include/cvs/logger/opencvHelper.hpp)
set(CPACK_DEBIAN_CVHELPER_PACKAGE_DEPENDS "libopencv-core${OpenCV_VERSION_MAJOR}.${OpenCV_VERSION_MINOR} (>= ${OpenCV_VERSION}+dfsg), libopencv-imgcodecs${OpenCV_VERSION_MAJOR}.${OpenCV_VERSION_MINOR} (>= ${OpenCV_VERSION}+dfsg)" PARENT_SCOPE)

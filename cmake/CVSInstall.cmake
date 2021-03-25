include(CMakeParseArguments)
include(CMakePackageConfigHelpers)

macro(cvs_install)
  set(options ENABLE_DEV)
  set(oneValueArgs VERSION NAME CONFIG)
  set(multiValueArgs TARGETS_BIN TARGETS_DEV HEADERS FILES_DEV FILES_BIN)
  cmake_parse_arguments(CVSINSTALL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT CVSINSTALL_CONFIG)
    set(CVSINSTALL_CONFIG "install/${PROJECT_NAME}Config.cmake.in")
  endif()

  if(NOT CVSINSTALL_VERSION)
    set(CVSINSTALL_VERSION ${PROJECT_VERSION})
  endif()

  if(NOT CVSINSTALL_NAME)
    set(CVSINSTALL_NAME ${PROJECT_NAME})
  endif()

  if(NOT CVSINSTALL_ENABLE_DEV)
    if(CVSINSTALL_TARGETS_DEV OR CVSINSTALL_FILES_DEV)
      set(CVSINSTALL_ENABLE_DEV ON)
    endif()
  endif()

  foreach(FILES_VAR "DEV" "BIN")
    list(LENGTH CVSINSTALL_FILES_${FILES_VAR} HEADERS_LENGTH)
    while(HEADERS_LENGTH GREATER 0)
      string(TOLOWER ${FILES_VAR} COMPONENT)

      list(GET CVSINSTALL_FILES_${FILES_VAR} 0 TAG)
      if(TAG STREQUAL "DESTINATION")
        math(EXPR INDEX "${INDEX} + 1" OUTPUT_FORMAT DECIMAL)
        list(GET CVSINSTALL_FILES_${FILES_VAR} 1 DESTINATION)
        list(SUBLIST CVSINSTALL_FILES_${FILES_VAR} 1 -1 CVSINSTALL_FILES_${FILES_VAR})
      endif()

      list(FIND CVSINSTALL_FILES_${FILES_VAR} "FILES" INDEX)
      list(GET CVSINSTALL_FILES_${FILES_VAR} ${INDEX} TAG)
      if(TAG STREQUAL "FILES")
        math(EXPR INDEX "${INDEX} + 1" OUTPUT_FORMAT DECIMAL)
      else()
        message(FATAL_ERROR "Expect \"FILES\" tag.")
      endif()

      list(FIND CVSINSTALL_FILES_${FILES_VAR} "DESTINATION" END)
      if(END GREATER -1)
        math(EXPR LENGHT "${END} - ${INDEX}")
      else()
        set(LENGHT ${END})
      endif()
      list(SUBLIST CVSINSTALL_FILES_${FILES_VAR} ${INDEX} ${LENGHT} FILES_LIST)

      install(FILES ${FILES_LIST}
        DESTINATION ${DESTINATION}
        COMPONENT ${COMPONENT})

      if(END EQUAL -1)
        break()
      endif()
      list(SUBLIST CVSINSTALL_FILES_${FILES_VAR} ${END} -1 CVSINSTALL_FILES_${FILES_VAR})
      list(LENGTH CVSINSTALL_FILES_${FILES_VAR} HEADERS_LENGTH)
    endwhile()
  endforeach()

  set(CVS_TARGETS OFF)

  if(CVSINSTALL_TARGETS_BIN)
    set(CVS_TARGETS ON)
    install(TARGETS ${CVSINSTALL_TARGETS_BIN}
      EXPORT  ${CVSINSTALL_NAME}Targets
      RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT   bin
      LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   bin
      ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   bin)
  endif()

  if(CVSINSTALL_TARGETS_DEV)
    set(CVS_TARGETS ON)
    install(TARGETS ${CVSINSTALL_TARGETS_DEV}
      EXPORT  ${CVSINSTALL_NAME}Targets
      RUNTIME
        DESTINATION ${CMAKE_INSTALL_BINDIR}
        COMPONENT   dev
      LIBRARY
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   dev
      ARCHIVE
        DESTINATION ${CMAKE_INSTALL_LIBDIR}
        COMPONENT   dev)
  endif()

  if(CVSINSTALL_ENABLE_DEV)
    if(CVS_TARGETS)
      install(EXPORT ${CVSINSTALL_NAME}Targets
        FILE         ${CVSINSTALL_NAME}Targets.cmake
        NAMESPACE    cvs::
        DESTINATION  ${CMAKE_INSTALL_LIBDIR}/cmake/${CVSINSTALL_NAME}
        COMPONENT    dev)
    endif()

    configure_package_config_file(${CVSINSTALL_CONFIG}
      ${CMAKE_CURRENT_BINARY_DIR}/cvsinstall/${CVSINSTALL_NAME}Config.cmake
      INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CVSINSTALL_NAME}
      NO_SET_AND_CHECK_MACRO NO_CHECK_REQUIRED_COMPONENTS_MACRO)

    write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/cvsinstall/${CVSINSTALL_NAME}ConfigVersion.cmake
      VERSION ${CVSINSTALL_VERSION}
      COMPATIBILITY SameMajorVersion)

    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cvsinstall/${CVSINSTALL_NAME}Config.cmake
      ${CMAKE_CURRENT_BINARY_DIR}/cvsinstall/${CVSINSTALL_NAME}ConfigVersion.cmake
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${CVSINSTALL_NAME}
      COMPONENT   dev)
  endif()
endmacro()

macro(cvs_package)
  set(options)
  set(oneValueArgs PACKAGE_NAME PACKAGE_VERSION)
  set(multiValueArgs)
  cmake_parse_arguments(CVSINSTALL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT CVSINSTALL_PACKAGE_NAME)
    string(TOLOWER ${PROJECT_NAME} CVSINSTALL_PACKAGE_NAME)
  endif()

  if(NOT CVSINSTALL_PACKAGE_VERSION)
    set(CVSINSTALL_PACKAGE_VERSION ${PROJECT_VERSION})
  endif()

  if(NOT CPACK_GENERATOR)
    set(CPACK_GENERATOR "DEB")
  endif()

  set(CPACK_DEBIAN_BIN_PACKAGE_NAME "${CVSINSTALL_PACKAGE_NAME}")

  set(CPACK_COMPONENT_UNSPECIFIED_GROUP "bin")
  set(CPACK_COMPONENT_BIN_GROUP "bin")
  set(CPACK_COMPONENT_DEV_GROUP "dev")

  set(CPACK_DEBIAN_DEV_PACKAGE_DEPENDS "${CVSINSTALL_PACKAGE_NAME} (= ${CVSINSTALL_PACKAGE_VERSION})")

  set(CPACK_PACKAGE_CONTACT "CVS")

  set(CPACK_DEB_COMPONENT_INSTALL ON)
  set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)

  include(CPack)
endmacro()

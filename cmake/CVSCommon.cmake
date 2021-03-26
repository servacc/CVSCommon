include_guard()

include(CMakeParseArguments)

include(CVSGitHelpers)

set(CVS_HOMEPAGE_URL "https://comvisionsys.ru")
set(CVS_ALIAS_NAME cvs)

macro(cvs_add_to_alias)
  add_library(${CVS_ALIAS_NAME}::${ARGV} ALIAS ${ARGV})
endmacro()

function(cvs_create_version_file)
  set(options)
  set(oneValueArgs NAME MAJOR MINOR PATCH OUTPUT)
  set(multiValueArgs)
  cmake_parse_arguments(CVSVERSION "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  configure_file("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/templates/version.hpp.in" "${CVSVERSION_OUTPUT}/version.hpp" @ONLY)
endfunction()

macro(cvs_library_version)
  set(options)
  set(oneValueArgs TARGET VERSION_MAJOR VERSION_MINOR)
  set(multiValueArgs)
  cmake_parse_arguments(CVSVERSION "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(${ARGC} EQUAL 1)
    set(CVSVERSION_TARGET ${ARGV})
  endif()

  if(NOT CVSVERSION_TARGET)
    message(SEND_ERROR "Target is not set.")
  endif()

  if(NOT CVSVERSION_VERSION_MAJOR)
    set(CVSVERSION_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CVSVERSION_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  endif()

  set_target_properties(${CVSVERSION_TARGET} PROPERTIES
    VERSION ${CVSVERSION_VERSION_MAJOR}.${CVSVERSION_VERSION_MINOR}
    SOVERSION ${CVSVERSION_VERSION_MAJOR})
endmacro()

macro(cvs_init_build_number)
  if(NOT BUILD_NUMBER)
    set(BUILD_NUMBER "")
  else()
    set(BUILD_NUMBER ".${BUILD_NUMBER}")
  endif()
endmacro()

macro(project)
  set(options CVS_PROJECT)
  set(oneValueArgs DESCRIPTION VERSION)
  set(multiValueArgs LANGUAGES)
  cmake_parse_arguments(CVSPROJECT "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT CVSPROJECT_CVS_PROJECT)
    _project(${ARGV})
  else()
    # TODO: set(CVS_${ARGV0}_VERSION ${VER}${DEV_OR_RC}+${HASH_OR_BUILDNUMBER})

    set(CVSPROJECT_VERSION_VALUE)
    set(CVSPROJECT_DESCRIPTION_VALUE)
    set(CVSPROJECT_LANGUAGES_VALUE)

    if(CVSPROJECT_VERSION)
      set(CVSPROJECT_VERSION_VALUE VERSION ${CVSPROJECT_VERSION})
    endif()

    if(CVSPROJECT_DESCRIPTION)
      cvs_git_current_hash(GIT_HASH)
      string(APPEND CVSPROJECT_DESCRIPTION_WITH_GIT ${CVSPROJECT_DESCRIPTION} " (${GIT_HASH})")
      set(CVSPROJECT_DESCRIPTION_VALUE DESCRIPTION ${CVSPROJECT_DESCRIPTION_WITH_GIT})
    endif()

    if(CVSPROJECT_LANGUAGES)
      set(CVSPROJECT_LANGUAGES_VALUE LANGUAGES ${CVSPROJECT_LANGUAGES})
    endif()

    _project(${ARGV0} ${CVSPROJECT_VERSION_VALUE}
      ${CVSPROJECT_DESCRIPTION_VALUE}
      ${CVSPROJECT_LANGUAGES_VALUE}
      HOMEPAGE_URL ${CVS_HOMEPAGE_URL})
  endif()
endmacro()

include(CMakeParseArguments)

set(CVS_HOMEPAGE_URL "https://comvisionsys.ru")

macro(cvs_library_version)
  set(options)
  set(oneValueArgs TARGET VERSION_MAJOR VERSION_MINOR)
  set(multiValueArgs)
  cmake_parse_arguments(CVSVERSION "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT CVSVERSION_TARGET)
    set(CVSVERSION_TARGET ${PROJECT_NAME})
  endif()

  if(NOT CVSVERSION_VERSION_MAJOR)
    set(CVSVERSION_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
    set(CVSVERSION_VERSION_MINOR ${PROJECT_VERSION_MINOR})
  endif()

  set_target_properties(${CVSVERSION_TARGET} PROPERTIES
    VERSION ${CVSVERSION_VERSION_MAJOR}.${CVSVERSION_VERSION_MINOR}
    SOVERSION ${CVSVERSION_VERSION_MAJOR})
endmacro()

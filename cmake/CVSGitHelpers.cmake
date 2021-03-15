macro(cvs_git_description DESCRIPTOR TAG COUNT)
    find_package(Git)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT_VARIABLE ${DESCRIPTOR}
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --abbrev=0
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT_VARIABLE ${TAG}
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process(
            COMMAND ${GIT_EXECUTABLE} rev-list ${GIT_TAG}..HEAD --count
            WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
            OUTPUT_VARIABLE ${COUNT}
            OUTPUT_STRIP_TRAILING_WHITESPACE)
endmacro()

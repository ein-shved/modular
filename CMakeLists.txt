cmake_minimum_required(VERSION 3.0)
project (modular)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

function(generate_modules MODULEFILE)
    execute_process(COMMAND echo ./gen_module.sh
                            ${CMAKE_CURRENT_SOURCE_DIR}/${MODULEFILE}.yaml
                            --builddir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
    execute_process(COMMAND ./gen_module.sh
                            ${CMAKE_CURRENT_SOURCE_DIR}/${MODULEFILE}.yaml
                            --builddir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE MODULE_FILES)

    string(REGEX MATCH "Generating ([^ \n\t]*)/[a-zA-Z0-9_]+\\.h\n" MODULE_PATH ${MODULE_FILES})
    set (${MODULEFILE}_INCLUDE_DIR ${CMAKE_MATCH_1} PARENT_SCOPE)
    string(REGEX MATCHALL "[a-zA-Z0-9_]+\\.h" MHEADERS ${MODULE_FILES})
    string(REGEX MATCHALL "[a-zA-Z0-9_]+\\.c" MSOURCES ${MODULE_FILES})
    set (${MODULEFILE}_MODULES PARENT_SCOPE)
    set (${MODULEFILE}_SOURCES ${MSOURCES} PARENT_SCOPE)
    set (${MODULEFILE}_HEADERS ${MHEADERS} PARENT_SCOPE)
    foreach (MOD ${MHEADERS})
        string (REPLACE ".h" "" MOD MOD)
        list (APPEND ${MODULEFILE}_MODULES ${MOD})
        set (${MOD}_SOURCES ${MODULE_PATH}/${MOD}.c PARENT_SCOPE)
        set (${MOD}_HEADERS ${MODULE_PATH}/${MOD}.h PARENT_SCOPE)
    endforeach()
endfunction(generate_modules)

include_directories(${PROJECT_SOURCE_DIR})

add_library(modular STATIC modular.c)

add_subdirectory(tests)

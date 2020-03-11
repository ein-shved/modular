cmake_minimum_required(VERSION 3.0)
project (modular)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

include_directories(${PROJECT_SOURCE_DIR})

add_library(modular STATIC modular.c)

function(generate_modules MODULEFILE)
    execute_process(COMMAND ./gen_module.sh -l -f=cmake -a
                            ${CMAKE_CURRENT_SOURCE_DIR}/${MODULEFILE}.yaml
                            --builddir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE MODULE_FILES)
    set (MODULEFILE_MODULES)
    set (MODULEFILE_HEADERS)
    set (MODULEFILE_SOURCES)
    set (MODULEFILE_INCLUDE_DIRS)
    foreach (F IN LISTS MODULE_FILES)
        if (F MATCHES "\\.h$")
            string(REGEX MATCH "([a-zA-Z0-9_]+)\\.h$" MT ${F})
            list(APPEND MODULEFILE_MODULES ${CMAKE_MATCH_1})
            set(MOD ${MODULEFILE}_${CMAKE_MATCH_1})
            string(REGEX MATCH "^([^ \n\t]*)/[a-zA-Z0-9_]+\\.h$" MODULE_PATH ${F})
            set (MOD_INCLUDE_DIR ${CMAKE_MATCH_1})
            set (${MOD}_INCLUDE_DIR ${MOD_INCLUDE_DIR} PARENT_SCOPE)
            list (APPEND MODULEFILE_INCLUDE_DIRS ${MOD_INCLUDE_DIR})
            set (${MOD}_HEADER ${F} PARENT_SCOPE)
            string(REGEX MATCH "([^ \n\t]*/[a-zA-Z0-9_]+)\\.h$" MODULE_PATH ${F})
            set (MOD_SOURCE "${CMAKE_MATCH_1}.c")
            set (MOD_HEADER ${F})
            list(APPEND MODULEFILE_HEADERS ${MOD_HEADER})
            list(APPEND MODULEFILE_SOURCES ${MOD_SOURCE})
            set (${MOD}_SOURCE ${MOD_SOURCE} PARENT_SCOPE)
            set (${MOD}_HEADER ${MOD_HEADER} PARENT_SCOPE)
        endif ()
    endforeach()
    add_custom_target(${MODULEFILE}_GEN
        COMMAND ./gen_module.sh
              ${CMAKE_CURRENT_SOURCE_DIR}/${MODULEFILE}.yaml
              --builddir ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${MODULEFILE}.yaml
        BYPRODUCTS ${${MODULEFILE}_HEADERS};${${MODULEFILE}_SOURCES})
    add_dependencies(${MODULEFILE} ${MODULEFILE}_GEN modular)
    target_include_directories(${MODULEFILE} PRIVATE ${MODULEFILE_INCLUDE_DIRS})
    target_sources(${MODULEFILE} PRIVATE ${MODULEFILE_SOURCES})

    set (${MODULEFILE}_MODULES ${MODULEFILE_MODULES} PARENT_SCOPE)
    set (${MODULEFILE}_HEADERS ${MODULEFILE_HEADERS} PARENT_SCOPE)
    set (${MODULEFILE}_SOURCES ${MODULEFILE_SOURCES} PARENT_SCOPE)
    set (${MODULEFILE}_INCLUDE_DIRS ${MODULEFILE_INCLUDE_DIRS} PARENT_SCOPE)
endfunction(generate_modules)

add_subdirectory(tests)

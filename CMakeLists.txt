cmake_minimum_required(VERSION 3.0)
project (modular)

set(CMAKE_BUILD_TYPE Debug)

if (DEFINED ENV{PREFIX})
    include_directories($ENV{PREFIX}/include)
    link_directories($ENV{PREFIX}/lib)
endif()

include_directories(${PROJECT_SOURCE_DIR})

if (AVR)
    add_avr_library(modular modular.c)
else()
    add_library(modular STATIC modular.c)
endif()


function(generate_modules TARGETNAME)
    set(options)
    set(oneValueArgs  YAML_FILE)
    set(multiValueArgs)
    cmake_parse_arguments(MODULE "${options}" "${oneValueArgs}"
                                 "${multiValueArgs}" ${ARGN})
    if (NOT DEFINED MODULE_YAML_FILE)
        set (MODULE_YAML_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/${TARGETNAME}.yaml")
    endif()
    execute_process(COMMAND ./gen_module.sh -l -f=cmake -a
                            ${MODULE_YAML_FILE}
                            --builddir ${CMAKE_BINARY_DIR}
                    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
                    OUTPUT_VARIABLE MODULE_FILES)
    set (TARGETNAME_MODULES)
    set (TARGETNAME_HEADERS)
    set (TARGETNAME_SOURCES)
    set (TARGETNAME_INCLUDE_DIRS)
    foreach (F IN LISTS MODULE_FILES)
        if (F MATCHES "\\.h$")
            string(REGEX MATCH "([a-zA-Z0-9_]+)\\.h$" MT ${F})
            list(APPEND TARGETNAME_MODULES ${CMAKE_MATCH_1})
            set(MOD ${TARGETNAME}_${CMAKE_MATCH_1})
            string(REGEX MATCH "^([^ \n\t]*)/[a-zA-Z0-9_]+\\.h$" MODULE_PATH ${F})
            set (MOD_INCLUDE_DIR ${CMAKE_MATCH_1})
            set (${MOD}_INCLUDE_DIR ${MOD_INCLUDE_DIR} PARENT_SCOPE)
            list (APPEND TARGETNAME_INCLUDE_DIRS ${MOD_INCLUDE_DIR})
            set (${MOD}_HEADER ${F} PARENT_SCOPE)
            string(REGEX MATCH "([^ \n\t]*/[a-zA-Z0-9_]+)\\.h$" MODULE_PATH ${F})
            set (MOD_SOURCE "${CMAKE_MATCH_1}.c")
            set (MOD_HEADER ${F})
            list(APPEND TARGETNAME_HEADERS ${MOD_HEADER})
            list(APPEND TARGETNAME_SOURCES ${MOD_SOURCE})
            set (${MOD}_SOURCES ${MOD_SOURCE} PARENT_SCOPE)
            set (${MOD}_HEADERS ${MOD_HEADER} PARENT_SCOPE)
        endif ()
    endforeach()
    add_custom_target(${TARGETNAME}_GEN
        COMMAND ./gen_module.sh
              ${CMAKE_CURRENT_SOURCE_DIR}/${TARGETNAME}.yaml
              --builddir ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
        SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${TARGETNAME}.yaml
        BYPRODUCTS ${TARGETNAME_SOURCES})
    if (NOT TARGET ${TARGETNAME})
        add_custom_target(${TARGETNAME})
    else()
        target_include_directories(${TARGETNAME} PRIVATE ${TARGETNAME_INCLUDE_DIRS})
    endif()
    add_dependencies(${TARGETNAME} ${TARGETNAME}_GEN modular)

    set (${TARGETNAME}_MODULAR_MODULES ${TARGETNAME_MODULES} PARENT_SCOPE)
    set (${TARGETNAME}_MODULAR_HEADERS ${TARGETNAME_HEADERS} PARENT_SCOPE)
    set (${TARGETNAME}_MODULAR_SOURCES ${TARGETNAME_SOURCES} PARENT_SCOPE)
    set (${TARGETNAME}_MODULAR_INCLUDE_DIRS ${TARGETNAME_INCLUDE_DIRS} PARENT_SCOPE)
endfunction(generate_modules)

add_subdirectory(util)
add_subdirectory(tests)

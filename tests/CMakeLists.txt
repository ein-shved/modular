cmake_minimum_required(VERSION 3.0)
project (modular_test)

include_directories("../")

FILE (GLOB PINGPONG_SOURCES *.c)

add_executable(pingpong ${PINGPONG_SOURCES} ../modular.c)

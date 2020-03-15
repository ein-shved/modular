
FILE (GLOB PINGPONG_SOURCES *.c)

add_executable(pingpong ${pingpong_SOURCES} pingpong.c)
generate_modules(pingpong)
include_directories(${pingpong_INCLUDE_DIR})
target_link_libraries(pingpong modular cmocka)

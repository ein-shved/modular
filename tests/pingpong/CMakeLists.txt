
FILE (GLOB PINGPONG_SOURCES *.c)

add_executable(pingpong pingpong.c)
generate_modules(pingpong)
target_sources(pingpong PRIVATE ${pingpong_MODULAR_SOURCES})
target_link_libraries(pingpong modular cmocka)

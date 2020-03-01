
FILE (GLOB PINGPONG_SOURCES *.c)

generate_modules(pingpong)
message( "PP ${pingpong_SOURCES}")
include_directories(${pingpong_INCLUDE_DIR})
add_executable(pingpong ${pingpong_SOURCES} pingpong.c)
target_link_libraries(pingpong modular)

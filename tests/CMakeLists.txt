if (AVR)
    add_subdirectory(util)
endif()

if (NOT AVR)
    add_subdirectory(pingpong)

    add_executable(test_ringbuf ringbuf.c)
    target_link_libraries(test_ringbuf ringbuf cmocka)
endif()

add_subdirectory(i2c)

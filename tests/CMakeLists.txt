if (AVR)
    add_subdirectory(util)
endif()

if (NOT AVR)
    add_subdirectory(pingpong)
endif()

add_subdirectory(i2c)

if (NOT AVR)
    add_subdirectory(pingpong)
endif()

if (AVR)
    add_subdirectory(i2c)
endif()

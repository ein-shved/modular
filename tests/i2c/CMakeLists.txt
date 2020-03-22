

generate_modules(i2c)
include_directories(${i2c_MODULAR_INCLUDE_DIRS})

add_avr_executable(master master.c ${i2c_light_SOURCES})
add_dependencies(${master_ELF_TARGET} i2c)
avr_target_link_libraries(master modular)

add_avr_executable(slave slave.c ${i2c_button_SOURCES})
add_dependencies(${slave_ELF_TARGET} i2c)
avr_target_link_libraries(slave modular)

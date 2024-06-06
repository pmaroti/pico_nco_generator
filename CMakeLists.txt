cmake_minimum_required(VERSION 3.13)


include(/home/maro/stuff/pico-sdk/pico_sdk_init.cmake)
project(nco)
pico_sdk_init()

add_executable(${PROJECT_NAME}
        nco.c
        ssd1306.c
        )

# pull in common dependencies
target_link_libraries(nco pico_stdlib hardware_pwm hardware_i2c)

# create map/bin/hex file etc.
pico_add_extra_outputs(${PROJECT_NAME})

pico_enable_stdio_uart(nco 0)
pico_enable_stdio_usb(nco 1)



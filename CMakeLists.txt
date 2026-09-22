cmake_minimum_required(VERSION 3.12)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(tros_pico_test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

link_directories($ENV{MICRO_ROS_PICO_DIR}/libmicroros)

add_executable(tros_pico_test
    src/tros_pico_test.c
    src/pico_uart_transport.c
)
target_link_libraries(tros_pico_test
    pico_stdlib
    hardware_pwm
    microros
)

target_include_directories(tros_pico_test PUBLIC
    $ENV{MICRO_ROS_PICO_DIR}/libmicroros/include
)

SET(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
SET(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")

# Configure Pico
pico_enable_stdio_usb(tros_pico_test 1)
pico_enable_stdio_uart(tros_pico_test 0)
add_compile_definitions(PICO_UART_ENABLE_CRLF_SUPPORT=0)
add_compile_definitions(PICO_STDIO_ENABLE_CRLF_SUPPORT=0)
add_compile_definitions(PICO_STDIO_DEFAULT_CRLF=0)

# Generate UF2
pico_add_extra_outputs(tros_pico_test)

cmake_minimum_required(VERSION 3.16)
project(stm32_canopen_modbus_gateway C)

set(CMAKE_C_STANDARD 99)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

add_executable(stm32_canopen_modbus_gateway
    src/main.c
)

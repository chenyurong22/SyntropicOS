cmake_minimum_required(VERSION 3.10)
project(ch32v307_demo C)

set(CMAKE_C_STANDARD 99)

add_executable(ch32v307_demo
    main.c
    board_evt.c
    ${CMAKE_SOURCE_DIR}/src/port/ch32v307/port_ch32v307.c
    ${CMAKE_SOURCE_DIR}/src/port/ch32v307/startup_ch32v307.c
)

target_compile_definitions(ch32v307_demo PRIVATE CH32V307xx=1)
target_include_directories(ch32v307_demo PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(ch32v307_demo PRIVATE SyntropicOS)

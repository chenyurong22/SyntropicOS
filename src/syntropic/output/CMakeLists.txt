target_sources(syntropic INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_buzzer.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_dshot.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_dshot_telemetry.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_led.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_smartled.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_soft_pwm.c
)

target_sources(syntropic INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_autotune.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_control_stats.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_flight_pid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_pid.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_rc_curve.c
    ${CMAKE_CURRENT_SOURCE_DIR}/syn_rc_failsafe.c
)

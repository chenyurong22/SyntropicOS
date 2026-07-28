/**
 * @file main_bare.c
 * @brief STM32 CANopen DS301 Slave Example (Bare-Metal while(1) Polling Loop Variant).
 *
 * Demonstrates bare-metal CANopen DS301 slave execution in a direct while(1) loop:
 * - Direct polling loop calling canopen_app_loop() and manual tick subtraction
 * - Periodic 100ms TPDO trigger checking (now - last_100ms >= 100)
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "syntropic/proto/syn_canopen.h"
#include "syntropic/port/syn_port_system.h"

extern void canopen_app_init(void);
extern void canopen_app_loop(uint32_t dt_ms);
extern void canopen_app_task_100ms(void);

int main_bare(void)
{
    canopen_app_init();

    uint32_t last_tick_ms  = syn_port_get_tick_ms();
    uint32_t last_100ms_ms = syn_port_get_tick_ms();

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();
        uint32_t dt_ms  = now_ms - last_tick_ms;
        if (dt_ms > 0) {
            last_tick_ms = now_ms;
            canopen_app_loop(dt_ms);
        }

        /* Periodic 100ms TPDO transmission check */
        if (now_ms - last_100ms_ms >= 100) {
            last_100ms_ms = now_ms;
            canopen_app_task_100ms();
        }
    }

    return 0;
}

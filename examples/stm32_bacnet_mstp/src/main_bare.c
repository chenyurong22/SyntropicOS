/**
 * @file main_bare.c
 * @brief STM32 BACnet MS/TP Sensor Node Example (Bare-Metal while(1) Polling Loop Variant).
 *
 * Demonstrates BACnet MS/TP frame decoding and Who-Is / ReadProperty responses
 * in a direct while(1) polling loop with manual tick subtraction.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "syntropic/proto/syn_bacnet.h"
#include "syntropic/port/syn_port_system.h"

extern void bacnet_app_init(void);
extern void bacnet_app_poll(void);
extern void bacnet_app_update_sensors(void);

int main_bare(void)
{
    bacnet_app_init();

    uint32_t last_sensor_ms = syn_port_get_tick_ms();

    while (1) {
        /* Direct BACnet MS/TP polling */
        bacnet_app_poll();

        /* Periodic 2000ms sensor COV update */
        uint32_t now_ms = syn_port_get_tick_ms();
        if (now_ms - last_sensor_ms >= 2000) {
            last_sensor_ms = now_ms;
            bacnet_app_update_sensors();
        }
    }

    return 0;
}

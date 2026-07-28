/**
 * @file main_bare.c
 * @brief STM32 EtherCAT Servo Drive Example (Bare-Metal while(1) Polling Loop Variant).
 *
 * Demonstrates 1kHz EtherCAT process data loop + CiA 402 drive state machine inside
 * a direct while(1) polling loop using manual tick subtraction.
 */

#include <stdbool.h>
#include <stdint.h>

#include "syntropic/proto/syn_ethercat.h"
#include "syntropic/port/syn_port_system.h"

extern void ethercat_app_init(void);
extern void ethercat_app_1khz_process(void);

int main_bare(void)
{
    ethercat_app_init();

    uint32_t last_1khz_ms = syn_port_get_tick_ms();

    while (1) {
        uint32_t now_ms = syn_port_get_tick_ms();

        /* Direct 1 ms / 1 kHz process data polling loop */
        if (now_ms - last_1khz_ms >= 1) {
            last_1khz_ms = now_ms;
            ethercat_app_1khz_process();
        }
    }

    return 0;
}

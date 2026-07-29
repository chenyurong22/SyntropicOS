/**
 * @file stm32_devicenet_example.c
 * @brief SyntropicOS STM32 ODVA DeviceNet (CIP over CAN) Slave Node Example.
 *
 * Demonstrates integrating a zero-heap DeviceNet slave node on STM32 using CAN.
 * Features Duplicate MAC ID check, CIP Object Model (Identity 0x01, DeviceNet 0x03,
 * Assembly 0x04), Polled I/O assembly data exchange, and QuickConnect™ hot-swapping.
 */

#include "syntropic/drivers/syn_can.h"
#include "syntropic/log/syn_log.h"
#include "syntropic/proto/syn_devicenet.h"
#include "syntropic/sched/syn_sched.h"

#include <stdbool.h>
#include <stdint.h>

#define STM32_DEVICENET_MAC_ID 0x06U /* DeviceNet Node Address: 6 */

/* DeviceNet Node Instance Context */
static SYN_DeviceNet_Node g_dnet;

/* Assembly Object (Class 0x04) Buffers */
static uint8_t g_sensor_inputs[4] = {0x01, 0x02, 0x03, 0x04};
static uint8_t g_actuator_outputs[4] = {0x00};

/* Simulated CAN hardware receive hook */
static void on_can_frame_received(uint32_t can_id, const uint8_t data[8], uint8_t len)
{
    uint32_t tx_can_id = 0;
    uint8_t tx_data[8] = {0};
    uint8_t tx_len = 0;

    /* Process incoming CAN frame through DeviceNet stack */
    if (syn_devicenet_on_can_rx(&g_dnet, can_id, data, len, &tx_can_id, tx_data, &tx_len)) {
        /* Transmit DeviceNet response frame over STM32 bxCAN / FDCAN driver */
        syn_can_send(0, tx_can_id, tx_data, tx_len);
    }
}

/* Periodic 10ms timer task servicing DeviceNet state machine */
static void app_10ms_task(void *arg)
{
    (void)arg;

    /* Advance DeviceNet timers (Duplicate MAC check & QuickConnect) */
    syn_devicenet_poll(&g_dnet, 10);

    /* Update simulated sensor input assembly data */
    g_sensor_inputs[0]++;
}

void stm32_devicenet_example_init(void)
{
    /* Initialize DeviceNet Node at MAC ID 6, 500kbps Baud Rate */
    syn_devicenet_init(&g_dnet, STM32_DEVICENET_MAC_ID, SYN_DEVICENET_BAUD_500K);

    /* Configure I/O Assembly Object buffers */
    syn_devicenet_set_assembly(&g_dnet, g_sensor_inputs, sizeof(g_sensor_inputs),
                               g_actuator_outputs, sizeof(g_actuator_outputs));

    /* Enable QuickConnect™ for fast hot-swap tool changing */
    syn_devicenet_set_quickconnect(&g_dnet, true);

    SYN_LOG_INFO(
        "STM32 DeviceNet Slave Node initialized at MAC ID: %u (Baud: 500k, QuickConnect: Enabled)",
        STM32_DEVICENET_MAC_ID);

    /* Register periodic 10ms timer task */
    syn_sched_register_task("dnet_task", app_10ms_task, NULL, 10);
}

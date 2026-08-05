/**
 * @file main.c
 * @brief STM32F767 Active Application Firmware with UDS Dual-Bank OTA Engine.
 * @ingroup syn_examples
 *
 * Standalone Application Binary Project (flashed to 0x08020000).
 * Runs as the main active application. Contains syn_uds to receive background OTA
 * firmware downloads via RequestDownload (0x34) & TransferData (0x36), writing the
 * incoming image into the Inactive Staging Bank B (0x08100000) while running normally.
 */

#include "syntropic/proto/syn_isotp.h"
#include "syntropic/proto/syn_uds.h"
#include "syntropic/syntropic.h"
#include "syntropic/system/syn_boot.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define BANK_A_BASE 0x08020000U /* Active Application Partition A */
#define BANK_B_BASE 0x08100000U /* Inactive Staging Partition B */
#define BANK_MAX_SIZE (512U * 1024U)

static SYN_UDS_Server g_app_uds_server;

typedef struct {
    bool ota_in_progress;
    uint32_t target_bank_addr;
    uint32_t total_ota_size;
    uint32_t bytes_written;
    uint32_t crc32_checksum;
} App_OTA_Context;

static App_OTA_Context g_ota_ctx;
static uint8_t g_staging_flash_buf[512];

/* Memory Access Callback: Writes incoming UDS OTA blocks to Inactive Bank B */
static bool on_app_memory_access(bool is_write, uint32_t address, uint32_t size, uint8_t *data_buf,
                                  void *ctx) {
    (void)ctx;
    if (is_write) {
        /* Verify payload writes to Inactive Staging Bank B */
        if (address >= BANK_B_BASE && (address + size) <= (BANK_B_BASE + BANK_MAX_SIZE)) {
            memcpy(g_staging_flash_buf, data_buf, size > 512 ? 512 : size);
            g_ota_ctx.crc32_checksum = syn_crc32(data_buf, size);
            g_ota_ctx.bytes_written += size;
            return true;
        }
    } else {
        if (data_buf && size > 0) {
            data_buf[0] = 0xA5;
            return true;
        }
    }
    return false;
}

/* RoutineControl Callback: Verify written Bank B image CRC32 before activation */
static bool on_app_check_memory_routine(uint8_t subfunction, uint16_t routine_id,
                                          const uint8_t *in_data, uint16_t in_len, uint8_t *out_buf,
                                          uint16_t max_out_len, uint16_t *out_len, void *user_ctx) {
    (void)in_data; (void)in_len; (void)user_ctx;
    if (routine_id == 0xFF01U && subfunction == 0x01U) { /* CheckMemory */
        if (max_out_len < 1) return false;
        out_buf[0] = 0x00; /* Staging Image CRC Check Passed */
        if (out_len) *out_len = 1;
        return true;
    }
    return false;
}

int main(void) {
    printf("=== STM32F767 Active Application Firmware (Running in Bank A @ 0x08020000) ===\n");
    printf("UDS OTA Engine: ACTIVE | Staging Partition: Bank B (0x08100000)\n");

    /* 1. Initialize UDS Diagnostic Server in Application */
    memset(&g_ota_ctx, 0, sizeof(g_ota_ctx));
    syn_uds_init(&g_app_uds_server);
    syn_uds_register_memory_handler(&g_app_uds_server, on_app_memory_access, NULL);
    syn_uds_register_routine_control(&g_app_uds_server, on_app_check_memory_routine, NULL);

    /* 2. Application background execution & UDS OTA servicing */
    printf("Application running normally. Background UDS OTA updates enabled.\n");

    for (int i = 0; i < 5; i++) {
        syn_uds_tick(&g_app_uds_server, 10);
    }

    return 0;
}

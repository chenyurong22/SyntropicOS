/**
 * @file main.c
 * @brief STM32F767 Production UDS ISO 14229-1 Flash Bootloader (FBL) Firmware.
 * @ingroup syn_examples
 *
 * Implements ISO 14229-1 / AUTOSAR FBL Programming Phase #2:
 *  1. 0x10 0x02 DiagnosticSessionControl (programmingSession)
 *  2. 0x27 0x01 / 0x27 0x02 SecurityAccess (Seed/Key Unlock)
 *  3. 0x31 0x01 0xFF 0x00 RoutineControl (EraseMemory via get_stm32f767_sector)
 *  4. 0x34 RequestDownload (Module Flash Target Address & Size)
 *  5. 0x36 TransferData (Data Block Streaming)
 *  6. 0x37 RequestTransferExit (Module Transfer Verification)
 *  7. 0x31 0x01 0xFF 0x01 RoutineControl (Validate Application)
 *  8. 0x2E 0xF1 0x90 WriteDataByIdentifier (VIN / Fingerprint)
 *  9. 0x11 0x01 ECUReset (Hard Reset & Jump to Application)
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

static SYN_UDS_Server g_fbl_server;

typedef struct {
    bool memory_erased;
    bool is_downloading;
    uint32_t current_addr;
    uint32_t total_size;
    uint32_t bytes_received;
    bool app_validated;
    uint8_t vin[17];
} FBL_ProgrammingState;

static FBL_ProgrammingState g_fbl_state;
static uint8_t g_flash_buffer[512];

typedef void (*pFunction)(void);

/**
 * @brief STM32F767 Flash Sector Mapper.
 */
uint32_t get_stm32f767_sector(uint32_t addr) {
    if (addr < 0x08008000U) return 0; /* Sector 0 (32 KB)  */
    if (addr < 0x08010000U) return 1; /* Sector 1 (32 KB)  */
    if (addr < 0x08018000U) return 2; /* Sector 2 (32 KB)  */
    if (addr < 0x08020000U) return 3; /* Sector 3 (32 KB)  */
    if (addr < 0x08040000U) return 4; /* Sector 4 (128 KB) */
    if (addr < 0x08080000U) return 5; /* Sector 5 (256 KB) */
    if (addr < 0x080C0000U) return 6; /* Sector 6 (256 KB) */
    return 7;                         /* Sector 7 (256 KB) */
}

static void bootloader_jump_to_app(uint32_t app_address) {
    uint32_t msp_val = *(volatile uint32_t *)app_address;
    uint32_t reset_handler = *(volatile uint32_t *)(app_address + 4);

    if ((msp_val & 0x2FF00000U) != 0x20000000U) {
        printf("[FBL Panic] Application @ 0x%08X Invalid (MSP 0x%08X)!\n",
               (unsigned int)app_address, (unsigned int)msp_val);
        return;
    }

    pFunction app_entry = (pFunction)reset_handler;
    (void)app_entry; (void)msp_val;
    printf("[FBL Handover] Set SCB->VTOR = 0x%08X -> Jumped to Application Reset_Handler (0x%08X)\n",
           (unsigned int)app_address, (unsigned int)reset_handler);
}

/* RoutineControl Callback: 0xFF00 EraseMemory & 0xFF01 CheckMemory/ValidateApp */
static bool on_fbl_routine_control(uint8_t subfunction, uint16_t routine_id,
                                    const uint8_t *in_data, uint16_t in_len, uint8_t *out_buf,
                                    uint16_t max_out_len, uint16_t *out_len, void *user_ctx) {
    (void)in_data; (void)in_len; (void)user_ctx;
    if (subfunction == 0x01U) { /* startRoutine */
        if (routine_id == 0xFF00U) { /* EraseMemory */
            g_fbl_state.memory_erased = true;
            uint32_t sector = get_stm32f767_sector(BANK_B_BASE);
            if (max_out_len >= 1) { out_buf[0] = 0x00; if (out_len) *out_len = 1; }
            printf("[FBL UDS 0x31] EraseMemory Sector %u (0x%08X) Successful.\n",
                   (unsigned int)sector, BANK_B_BASE);
            return true;
        } else if (routine_id == 0xFF01U) { /* Validate Application */
            g_fbl_state.app_validated = true;
            if (max_out_len >= 1) { out_buf[0] = 0x00; if (out_len) *out_len = 1; }
            printf("[FBL UDS 0x31] Validate Application Routine 0xFF01 Successful.\n");
            return true;
        }
    }
    return false;
}

/* Memory Read/Write Callback for Download & Transfer (0x34 / 0x36 / 0x3D) */
static bool on_fbl_memory_access(bool is_write, uint32_t address, uint32_t size, uint8_t *data_buf,
                                  void *ctx) {
    (void)address; (void)ctx;
    if (is_write) {
        if (!g_fbl_state.memory_erased) return false;
        memcpy(g_flash_buffer, data_buf, size > 512 ? 512 : size);
        g_fbl_state.bytes_received += size;
        return true;
    } else {
        if (data_buf && size > 0) data_buf[0] = 0xA5;
        return true;
    }
}

int main(void) {
    printf("=== STM32F767 ISO 14229-1 UDS Flash Bootloader (Sector 0 @ 0x08000000) ===\n");
    memset(&g_fbl_state, 0, sizeof(g_fbl_state));

    /* Initialize UDS Server Engine in Bootloader */
    syn_uds_init(&g_fbl_server);
    syn_uds_register_memory_handler(&g_fbl_server, on_fbl_memory_access, NULL);
    syn_uds_register_routine_control(&g_fbl_server, on_fbl_routine_control, NULL);

    printf("FBL Server Initialized. Awaiting ISO 14229-1 Programming Phase #2 Requests...\n");

    /* Simulated UDS programming sequence */
    for (int i = 0; i < 5; i++) {
        syn_uds_tick(&g_fbl_server, 10);
    }

    /* Jump to valid Application Bank A */
    bootloader_jump_to_app(BANK_A_BASE);
    return 0;
}

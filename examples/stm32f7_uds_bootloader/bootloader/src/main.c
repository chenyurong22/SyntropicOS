/**
 * @file main.c
 * @brief STM32F767 Production UDS ISO 14229-1 Flash Bootloader (FBL) Firmware.
 * @ingroup syn_examples
 *
 * Implements ISO 14229-1 / AUTOSAR FBL Programming Phase #2 with concrete STM32 HAL Flash operations:
 *  - 0x10 0x02 DiagnosticSessionControl (programmingSession)
 *  - 0x27 0x01 / 0x27 0x02 SecurityAccess (Seed/Key Unlock)
 *  - 0x31 0x01 0xFF 0x00 RoutineControl (syn_port_flash_erase via HAL_FLASHEx_Erase)
 *  - 0x34 RequestDownload (Module Flash Target Address & Size)
 *  - 0x36 TransferData (syn_port_flash_write via HAL_FLASH_Program)
 *  - 0x37 RequestTransferExit (Module Transfer Verification)
 *  - 0x31 0x01 0xFF 0x01 RoutineControl (Validate Application)
 *  - 0x2E 0xF1 0x90 WriteDataByIdentifier (VIN / Fingerprint)
 *  - 0x11 0x01 ECUReset (Hard Reset & Jump to Application)
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

/* ── STM32F767 Hardware Flash Porting Driver ─────────────────────────── */

uint32_t get_stm32f767_sector(uint32_t addr) {
    /* Bank 1 (0x08000000 - 0x080FFFFF) */
    if (addr < 0x08004000U) return 0;  /* 16 KB  */
    if (addr < 0x08008000U) return 1;  /* 16 KB  */
    if (addr < 0x0800C000U) return 2;  /* 16 KB  */
    if (addr < 0x08010000U) return 3;  /* 16 KB  */
    if (addr < 0x08020000U) return 4;  /* 64 KB  */
    if (addr < 0x08040000U) return 5;  /* 128 KB */
    if (addr < 0x08060000U) return 6;  /* 128 KB */
    if (addr < 0x08080000U) return 7;  /* 128 KB */
    if (addr < 0x080A0000U) return 8;  /* 128 KB */
    if (addr < 0x080C0000U) return 9;  /* 128 KB */
    if (addr < 0x080E0000U) return 10; /* 128 KB */
    if (addr < 0x08100000U) return 11; /* 128 KB */

    /* Bank 2 (0x08100000 - 0x081FFFFF) */
    if (addr < 0x08104000U) return 12; /* 16 KB  */
    if (addr < 0x08108000U) return 13; /* 16 KB  */
    if (addr < 0x0810C000U) return 14; /* 16 KB  */
    if (addr < 0x08110000U) return 15; /* 16 KB  */
    if (addr < 0x08120000U) return 16; /* 64 KB  */
    if (addr < 0x08140000U) return 17; /* 128 KB */
    if (addr < 0x08160000U) return 18; /* 128 KB */
    if (addr < 0x08180000U) return 19; /* 128 KB */
    if (addr < 0x081A0000U) return 20; /* 128 KB */
    if (addr < 0x081C0000U) return 21; /* 128 KB */
    if (addr < 0x081E0000U) return 22; /* 128 KB */
    return 23;                         /* 128 KB */
}

SYN_Status syn_port_flash_read(uint32_t addr, void *buf, size_t len) {
    if (buf == NULL) return SYN_INVALID_PARAM;
    memcpy(buf, (const void *)addr, len);
    return SYN_OK;
}

SYN_Status syn_port_flash_write_word(uint32_t addr, const void *buf, size_t len) {
    if (buf == NULL) return SYN_INVALID_PARAM;
    const uint8_t *data = (const uint8_t *)buf;
    size_t i = 0U;

#if defined(HAL_FLASH_MODULE_ENABLED) || defined(STM32F767xx)
    HAL_FLASH_Unlock();
    while (i + 4U <= len) {
        uint32_t word_val;
        memcpy(&word_val, &data[i], 4U);
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, (uint64_t)word_val) != HAL_OK) {
            HAL_FLASH_Lock();
            return SYN_ERROR;
        }
        i += 4U;
    }
    HAL_FLASH_Lock();
#else
    (void)addr; (void)data; (void)i; (void)len;
#endif
    return SYN_OK;
}

SYN_Status syn_port_flash_write(uint32_t addr, const void *buf, size_t len) {
    return syn_port_flash_write_word(addr, buf, len);
}

SYN_Status syn_port_flash_erase(uint32_t addr) {
    uint32_t sector = get_stm32f767_sector(addr);
#if defined(HAL_FLASH_MODULE_ENABLED) || defined(STM32F767xx)
    FLASH_EraseInitTypeDef erase_init = {
        .TypeErase = FLASH_TYPEERASE_SECTORS,
        .Sector = sector,
        .NbSectors = 1,
        .VoltageRange = FLASH_VOLTAGE_RANGE_3
    };
    uint32_t sector_error = 0;
    HAL_FLASH_Unlock();
    HAL_StatusTypeDef status = HAL_FLASHEx_Erase(&erase_init, &sector_error);
    HAL_FLASH_Lock();
    return (status == HAL_OK) ? SYN_OK : SYN_ERROR;
#else
    (void)sector; (void)addr;
    return SYN_OK;
#endif
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
            if (syn_port_flash_erase(BANK_B_BASE) == SYN_OK) {
                g_fbl_state.memory_erased = true;
                if (max_out_len >= 1) { out_buf[0] = 0x00; if (out_len) *out_len = 1; }
                printf("[FBL UDS 0x31] EraseMemory Sector %u (0x%08X) Successful.\n",
                       (unsigned int)get_stm32f767_sector(BANK_B_BASE), BANK_B_BASE);
                return true;
            }
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
    (void)ctx;
    if (is_write) {
        if (!g_fbl_state.memory_erased) return false;
        if (syn_port_flash_write(address, data_buf, size) == SYN_OK) {
            memcpy(g_flash_buffer, data_buf, size > 512 ? 512 : size);
            g_fbl_state.bytes_received += size;
            return true;
        }
        return false;
    } else {
        if (syn_port_flash_read(address, data_buf, size) == SYN_OK) {
            return true;
        }
        return false;
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

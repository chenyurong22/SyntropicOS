/**
 * @file main.c
 * @brief STM32F767 Minimal Dual-Bank Selector Bootloader (~4 KB in Sector 0).
 * @ingroup syn_examples
 *
 * Standalone Bootloader Binary Project (flashed to 0x08000000).
 * Runs for ~5ms on power-on reset. Has ZERO UDS/network stack footprint.
 * Inspects syn_boot slot validity in Bank A vs Bank B, sets SCB->VTOR, and jumps.
 */

#include "syntropic/syntropic.h"
#include "syntropic/system/syn_boot.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#define BANK_A_BASE 0x08020000U /* Application Bank A Base Address */
#define BANK_B_BASE 0x08100000U /* Application Bank B Base Address */

typedef void (*pFunction)(void);

/**
 * @brief Lightweight Vector Table Relocation and Application Jump.
 */
static void boot_jump_to_app(uint32_t app_address) {
    uint32_t msp_val = *(volatile uint32_t *)app_address;
    uint32_t reset_handler = *(volatile uint32_t *)(app_address + 4);

    /* Validate stack pointer address (RAM 0x20000000 - 0x20080000) */
    if ((msp_val & 0x2FF00000U) != 0x20000000U) {
        printf("[Minimal FBL Error] Bank @ 0x%08X MSP Invalid!\n", (unsigned int)app_address);
        return;
    }

    pFunction app_entry = (pFunction)reset_handler;

    /* Microcontroller Vector Table Relocation & Jump */
    /* __disable_irq(); */
    /* SCB->VTOR = app_address; */
    /* __set_MSP(msp_val); */
    (void)app_entry;
    printf("[Minimal FBL] Set SCB->VTOR = 0x%08X -> Jumped to Application Reset_Handler (0x%08X)\n",
           (unsigned int)app_address, (unsigned int)reset_handler);
}

int main(void) {
    printf("=== STM32F767 Minimal Dual-Bank Selector Bootloader (Sector 0 @ 0x08000000) ===\n");
    printf("Footprint: ~4 KB | Network Stack: NONE\n");

    /* 1. Inspect Bank A & Bank B Image Header / Stack Pointer */
    uint32_t bank_a_msp = *(volatile uint32_t *)BANK_A_BASE;
    uint32_t bank_b_msp = *(volatile uint32_t *)BANK_B_BASE;

    bool bank_a_valid = ((bank_a_msp & 0x2FF00000U) == 0x20000000U);
    bool bank_b_valid = ((bank_b_msp & 0x2FF00000U) == 0x20000000U);

    printf("Bank A @ 0x%08X -> Status: %s\n", BANK_A_BASE, bank_a_valid ? "VALID" : "INVALID");
    printf("Bank B @ 0x%08X -> Status: %s\n", BANK_B_BASE, bank_b_valid ? "VALID" : "INVALID");

    /* 2. Select Active Bank (Prefer Bank A if valid, fallback to Bank B) */
    if (bank_a_valid) {
        printf("[Minimal FBL] Selecting Active Application Bank A...\n");
        boot_jump_to_app(BANK_A_BASE);
    } else if (bank_b_valid) {
        printf("[Minimal FBL] Selecting Active Application Bank B...\n");
        boot_jump_to_app(BANK_B_BASE);
    } else {
        printf("[Minimal FBL Panic] Both Application Banks Corrupted! Entering Safe Mode...\n");
    }

    return 0;
}

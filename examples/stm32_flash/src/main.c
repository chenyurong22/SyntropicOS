/**
 * @file main.c
 * @brief STM32 Register-Level Flash Memory Programming & Persistent Storage Example.
 *
 * Demonstrates bare-metal Flash memory operations on STM32 (Cortex-M) targets:
 * - Flash Control Register (CR) unlock & lock sequences
 * - Busy flag (BSY) status polling
 * - Sector / Page erase operations
 * - 32-bit word programming and readback verification
 * - Integration with SyntropicOS syn_port_flash port layer
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "syntropic/syntropic.h"
#include "syntropic/port/syn_port_flash.h"

/* ── STM32 Hardware Register Definitions ─────────────────────────────────── */
#define FLASH_BASE_ADDR      0x40023C00U

typedef struct {
    volatile uint32_t ACR;     /* Access Control Register      (0x00) */
    volatile uint32_t KEYR;    /* Key Register                 (0x04) */
    volatile uint32_t OPTKEYR; /* Option Key Register          (0x08) */
    volatile uint32_t SR;      /* Status Register              (0x0C) */
    volatile uint32_t CR;      /* Control Register             (0x10) */
    volatile uint32_t OPTCR;   /* Option Control Register      (0x14) */
} STM32_FLASH_TypeDef;

#define STM32_FLASH          ((STM32_FLASH_TypeDef *)FLASH_BASE_ADDR)

/* Unlock Keys */
#define FLASH_KEY1           0x45670123U
#define FLASH_KEY2           0xCDEF89ABU

/* Flash CR Control Bits */
#define FLASH_CR_PG          (1U << 0)  /* Programming Enable          */
#define FLASH_CR_SER         (1U << 1)  /* Sector Erase Enable         */
#define FLASH_CR_MER         (1U << 2)  /* Mass Erase Enable           */
#define FLASH_CR_STRT        (1U << 16) /* Start Erase Operation       */
#define FLASH_CR_LOCK        (1U << 31) /* Flash CR Lock Bit           */

/* Program parallelism (32-bit parallelism at 2.7V - 3.6V) */
#define FLASH_CR_PSIZE_32    (2U << 8)

/* Flash SR Status Bits */
#define FLASH_SR_BSY         (1U << 16) /* Flash Busy Flag             */
#define FLASH_SR_EOP         (1U << 0)  /* End of Operation            */

/* Target Test Address (Sector 5 on STM32F407) */
#define TEST_FLASH_SECTOR    5
#define TEST_FLASH_ADDR      0x08020000U

/* ── Bare-Metal Flash Driver Functions ──────────────────────────────────── */

/**
 * @brief Unlock the Flash Control Register (CR).
 */
static void stm32_flash_unlock(void)
{
    if ((STM32_FLASH->CR & FLASH_CR_LOCK) != 0U) {
        STM32_FLASH->KEYR = FLASH_KEY1;
        STM32_FLASH->KEYR = FLASH_KEY2;
    }
}

/**
 * @brief Lock the Flash Control Register (CR).
 */
static void stm32_flash_lock(void)
{
    STM32_FLASH->CR |= FLASH_CR_LOCK;
}

/**
 * @brief Poll until any active Flash write/erase operation completes.
 */
static void stm32_flash_wait_busy(void)
{
    while ((STM32_FLASH->SR & FLASH_SR_BSY) != 0U) {
        /* Busy wait loop */
    }
}

/**
 * @brief Erase an STM32 Flash sector at the hardware register level.
 */
static bool stm32_flash_erase_sector(uint8_t sector_num)
{
    stm32_flash_wait_busy();
    stm32_flash_unlock();

    /* Clear any pending status/error flags */
    STM32_FLASH->SR = 0xFFFFU;

    /* Configure Sector Erase mode and 32-bit parallelism */
    uint32_t cr = STM32_FLASH->CR;
    cr &= ~(0x0FU << 3);              /* Clear sector selection bits [6:3] */
    cr |= ((uint32_t)sector_num << 3); /* Set target sector index */
    cr |= FLASH_CR_SER;               /* Enable Sector Erase */
    cr |= FLASH_CR_PSIZE_32;          /* Select 32-bit width */
    STM32_FLASH->CR = cr;

    /* Start erase */
    STM32_FLASH->CR |= FLASH_CR_STRT;

    stm32_flash_wait_busy();

    /* Disable Sector Erase mode */
    STM32_FLASH->CR &= ~FLASH_CR_SER;

    stm32_flash_lock();
    return true;
}

/**
 * @brief Write a 32-bit word directly to Flash memory.
 */
static bool stm32_flash_write_word(uint32_t address, uint32_t data)
{
    if ((address % 4U) != 0U) {
        return false; /* Must be 4-byte aligned */
    }

    stm32_flash_wait_busy();
    stm32_flash_unlock();

    /* Enable programming mode */
    STM32_FLASH->CR &= ~(3U << 8);
    STM32_FLASH->CR |= FLASH_CR_PSIZE_32;
    STM32_FLASH->CR |= FLASH_CR_PG;

    /* Write 32-bit word */
    *(volatile uint32_t *)address = data;

    stm32_flash_wait_busy();

    /* Disable programming mode */
    STM32_FLASH->CR &= ~FLASH_CR_PG;

    stm32_flash_lock();

    /* Readback verification */
    return (*(volatile uint32_t *)address == data);
}

/* ── SyntropicOS Hardware Flash Port Implementation ──────────────────────── */

#include <stdio.h>
#include "syntropic/syntropic.h"
#include "syntropic/port/syn_port_flash.h"

SYN_Status syn_port_flash_erase(uint32_t addr)
{
    (void)addr;
    /* Calculate sector from address (simplified example for Sector 5) */
    uint8_t sector = 5;
    return stm32_flash_erase_sector(sector) ? SYN_OK : SYN_ERROR;
}

SYN_Status syn_port_flash_read(uint32_t addr, void *buf, size_t len)
{
    if (buf == NULL) return SYN_INVALID_PARAM;
    memcpy(buf, (const void *)addr, len);
    return SYN_OK;
}

SYN_Status syn_port_flash_write(uint32_t addr, const void *buf, size_t len)
{
    if (buf == NULL || (addr % 4U) != 0 || (len % 4U) != 0) {
        return SYN_INVALID_PARAM;
    }

    const uint32_t *src = (const uint32_t *)buf;
    size_t words = len / 4;

    for (size_t i = 0; i < words; i++) {
        if (!stm32_flash_write_word(addr + (uint32_t)(i * 4), src[i])) {
            return SYN_ERROR;
        }
    }

    return SYN_OK;
}

uint32_t syn_port_flash_sector_size(uint32_t addr)
{
    (void)addr;
    return 128u * 1024u; /* 128 KB sector size on STM32F4 Sector 5 */
}

/* ── Application Entry Point ─────────────────────────────────────────────── */

int main(void)
{
    printf("=== SyntropicOS Bare-Metal STM32 Flash Register Example ===\n");

    /* 1. Register-Level Sector Erase */
    printf("Erasing Flash Sector %d...\n", TEST_FLASH_SECTOR);
    if (!stm32_flash_erase_sector(TEST_FLASH_SECTOR)) {
        printf("Flash Sector Erase FAILED!\n");
        return -1;
    }
    printf("Flash Sector Erase SUCCESSFUL!\n");

    /* 2. Register-Level Word Programming */
    uint32_t magic_word = 0x594E5452; /* 'SNTR' in ASCII */
    printf("Programming 0x%08lX to Flash 0x%08lX...\n", (unsigned long)magic_word, (unsigned long)TEST_FLASH_ADDR);
    if (!stm32_flash_write_word(TEST_FLASH_ADDR, magic_word)) {
        printf("Flash Word Programming FAILED!\n");
        return -1;
    }

    /* 3. Readback Verification */
    uint32_t readback = *(volatile uint32_t *)TEST_FLASH_ADDR;
    printf("Flash Readback: 0x%08lX (%s)\n", (unsigned long)readback,
           (readback == magic_word) ? "VERIFIED PASS" : "FAIL");

    /* 4. SyntropicOS Flash Port Layer Verification */
    uint32_t test_data[4] = {0x11223344, 0x55667788, 0x99AABBCC, 0xDDEEFF00};
    uint32_t read_buf[4] = {0};

    printf("Testing SyntropicOS syn_port_flash_write()...\n");
    syn_port_flash_write(TEST_FLASH_ADDR + 16, test_data, sizeof(test_data));
    syn_port_flash_read(TEST_FLASH_ADDR + 16, read_buf, sizeof(read_buf));

    if (memcmp(test_data, read_buf, sizeof(test_data)) == 0) {
        printf("SyntropicOS Flash Port Integration PASS!\n");
    }

    return 0;
}

/*
 * Linker Script for STM32F767 dual-bank UDS bootloader partition architecture.
 */

MEMORY
{
    FLASH_FBL (rx)  : ORIGIN = 0x08000000, LENGTH = 128K  /* Sector 0..3 (Bootloader) */
    FLASH_BANK_A (rx): ORIGIN = 0x08020000, LENGTH = 512K  /* Sector 4..7 (Bank A)     */
    FLASH_BANK_B (rx): ORIGIN = 0x08100000, LENGTH = 512K  /* Sector 12..15 (Bank B)   */
    SRAM (rwx)      : ORIGIN = 0x20000000, LENGTH = 512K  /* System SRAM */
}

ENTRY(Reset_Handler)

SECTIONS
{
    /* Position SYN_FBL_AppHeader at origin of application partition */
    .app_header :
    {
        . = ALIGN(4);
        KEEP(*(.app_header))
        . = ALIGN(4);
    } > FLASH_BANK_A

    .text :
    {
        . = ALIGN(4);
        *(.isr_vector)
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH_BANK_A

    .data : AT (_etext)
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > SRAM

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > SRAM
}

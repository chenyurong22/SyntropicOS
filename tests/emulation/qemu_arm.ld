/* Linker script for ARM Cortex-M4 QEMU MPS2-AN385 board */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 4M
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 4M
}

_estack = ORIGIN(RAM) + LENGTH(RAM);

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.init)
        *(.fini)
        *(.ARM.extab*)
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
        *(.rodata*)
        _etext = .;
    } > FLASH

    .data : AT (_etext)
    {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM

    .bss :
    {
        _sbss = .;
        __bss_start__ = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
        __bss_end__ = .;
        end = .;
        __end__ = .;
    } > RAM
}

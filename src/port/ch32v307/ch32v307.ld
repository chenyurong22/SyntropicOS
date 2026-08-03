/* Linker script for WCH CH32V307VCT6 (QingKe V4F RISC-V Core) */
ENTRY(Reset_Handler)

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 64K
}

SECTIONS
{
  .vector_table :
  {
    . = ALIGN(4);
    KEEP(*(.vector_table))
    . = ALIGN(4);
  } > FLASH

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text.*)
    *(.rodata)
    *(.rodata.*)
    . = ALIGN(4);
    _etext = .;
  } > FLASH

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data.*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
  } > RAM

  .stack (NOLOAD) :
  {
    . = ALIGN(8);
    . += 0x1000; /* 4 KB Stack */
    . = ALIGN(8);
    _estack = .;
  } > RAM
}

/**
 * @file startup_ch32v307.c
 * @brief Bare-metal startup code and vector table for WCH CH32V307VCT6 (QingKe V4F RISC-V).
 */

#include <stdint.h>

/** @brief Start address of initialized data section in Flash. */
extern uint32_t _sidata;
/** @brief Start address of initialized data section in RAM. */
extern uint32_t _sdata;
/** @brief End address of initialized data section in RAM. */
extern uint32_t _edata;
/** @brief Start address of uninitialized data section in RAM. */
extern uint32_t _sbss;
/** @brief End address of uninitialized data section in RAM. */
extern uint32_t _ebss;
/** @brief Initial top of stack address in RAM. */
extern uint32_t _estack;

/**
 * @brief Main application entry point function declaration.
 * @return Exit status code.
 */
extern int main(void);

/**
 * @brief MCU Reset handler — initializes memory sections and jumps to main().
 */
void Reset_Handler(void);

#if defined(__riscv)
#define SYN_ISR_ATTR __attribute__((interrupt("WCH-Interrupt-fast")))
#else
#define SYN_ISR_ATTR
#endif

/**
 * @brief Default unhandled exception/interrupt catch handler.
 */
SYN_ISR_ATTR void Default_Handler(void);

/**
 * @brief Non-Maskable Interrupt (NMI) handler.
 */
SYN_ISR_ATTR void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));

/**
 * @brief HardFault exception handler.
 */
SYN_ISR_ATTR void HardFault_Handler(void) __attribute__((weak, alias("Default_Handler")));

/**
 * @brief SysTick timer interrupt handler.
 */
SYN_ISR_ATTR void SysTick_Handler(void) __attribute__((weak, alias("Default_Handler")));

/**
 * @brief USART1 serial communication interrupt handler.
 */
SYN_ISR_ATTR void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

/**
 * @brief CAN1 receive mailbox 0 interrupt handler.
 */
SYN_ISR_ATTR void CAN1_RX0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

/**
 * @brief CH32V307 interrupt vector table allocated in .vector_table section.
 */
__attribute__((section(".vector_table"), used)) void (*const vector_table[])(void) = {
    Reset_Handler,
    NMI_Handler,
    HardFault_Handler,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    0,
    SysTick_Handler,
    0,
    0,
    USART1_IRQHandler,
    CAN1_RX0_IRQHandler};

void Reset_Handler(void)
{
    /* Initialize Stack Pointer sp */
    __asm__ volatile("la sp, _estack");

    /* Copy .data section from Flash to RAM */
    uint32_t *src = &_sidata;
    uint32_t *dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* Clear .bss section in RAM */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0U;
    }

    /* Call application main */
    (void)main();

    /* Infinite loop if main returns */
    while (1) {
    }
}

SYN_ISR_ATTR void Default_Handler(void)
{
    while (1) {
    }
}

/**
 * @file startup_cortexm4.c
 * @brief Minimal bare-metal Cortex-M4 vector table and semihosting startup for QEMU mps2-an385.
 */

#include <stdint.h>

extern uint32_t _estack;
extern uint32_t _etext;
extern uint32_t _sdata;
extern uint32_t _edata;
extern uint32_t _sbss;
extern uint32_t _ebss;

extern int main(void);
extern void initialise_monitor_handles(void);

void Reset_Handler(void);
void Default_Handler(void);

/* Vector Table for ARM Cortex-M4 */
__attribute__((section(".isr_vector"))) void (*const g_pfnVectors[])(void) = {
    (void (*)(void))((uint32_t)&_estack),
    Reset_Handler,
    Default_Handler, /* NMI */
    Default_Handler, /* HardFault */
    Default_Handler, /* MemManage */
    Default_Handler, /* BusFault */
    Default_Handler, /* UsageFault */
};

void Default_Handler(void)
{
    while (1)
        ;
}

static inline void semihosting_exit(int code)
{
    uint32_t arg[2] = {0x20026, (uint32_t)code}; /* ADP_Stopped_ApplicationExit */
    __asm__ volatile("mov r0, #0x18\n"
                     "mov r1, %0\n"
                     "bkpt 0xab\n"
                     :
                     : "r"(arg)
                     : "r0", "r1", "memory");
}

void Reset_Handler(void)
{
    /* Copy initialized data from FLASH to RAM */
    uint32_t *pSrc = &_etext;
    uint32_t *pDst = &_sdata;
    while (pDst < &_edata) {
        *pDst++ = *pSrc++;
    }

    /* Zero fill the .bss segment in RAM */
    uint32_t *pBss = &_sbss;
    while (pBss < &_ebss) {
        *pBss++ = 0;
    }

    /* Initialize semihosting I/O for printf/putchar */
    initialise_monitor_handles();

    /* Call main */
    int ret = main();

    /* Exit QEMU semihosting cleanly */
    semihosting_exit(ret);

    while (1)
        ;
}

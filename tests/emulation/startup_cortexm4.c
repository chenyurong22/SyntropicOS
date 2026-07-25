/**
 * @file startup_cortexm4.c
 * @brief Minimal bare-metal Cortex-M4 vector table and semihosting startup for QEMU mps2-an385.
 */

#include <stdint.h>

extern uint32_t _estack;
extern int main(void);
extern void initialise_monitor_handles(void);

void Reset_Handler(void);
void Default_Handler(void);

/* Vector Table for ARM Cortex-M4 */
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
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
    while (1);
}

void Reset_Handler(void)
{
    /* Initialize semihosting I/O for printf/putchar */
    initialise_monitor_handles();

    /* Call main */
    main();

    while (1);
}

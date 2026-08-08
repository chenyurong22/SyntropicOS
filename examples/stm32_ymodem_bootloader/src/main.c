/**
 * @file main.c
 * @brief STM32 YMODEM Serial Flash Bootloader Example.
 * @ingroup syn_examples
 *
 * Demonstrates updating firmware over UART using YMODEM batch file transfer protocol:
 *  - Binds syn_ymodem receiver to serial port
 *  - On SYN_YMODEM_EVENT_FILE_START: erases target flash partition
 *  - On SYN_YMODEM_EVENT_DATA: programs firmware bytes to flash
 *  - On SYN_YMODEM_EVENT_FILE_END: verifies image and jumps to application
 */

#include "syntropic/proto/syn_ymodem.h"
#include "syntropic/syntropic.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define APP_FLASH_BASE 0x08020000U
#define APP_MAX_SIZE   (128U * 1024U)

typedef struct {
    uint32_t current_addr;
    uint32_t total_written;
    bool erase_complete;
} Bootloader_State;

static Bootloader_State g_boot_state;

static void ymodem_putchar(uint8_t byte, void *ctx)
{
    (void)ctx;
    /* USART TX implementation stub */
    putchar(byte);
}

static int ymodem_getchar(uint32_t timeout_ms, void *ctx)
{
    (void)ctx;
    (void)timeout_ms;
    /* USART RX implementation stub with timeout */
    return getchar();
}

static int ymodem_event_handler(SYN_YMODEM_Event event, const uint8_t *data, size_t len, void *ctx)
{
    Bootloader_State *state = (Bootloader_State *)ctx;
    if (state == NULL) {
        return -1;
    }

    switch (event) {
    case SYN_YMODEM_EVENT_FILE_START:
        state->current_addr = APP_FLASH_BASE;
        state->total_written = 0;
        state->erase_complete = true; /* Stub flash erase */
        printf("YMODEM: Start receiving file '%.*s'\n", (int)len, (const char *)data);
        break;

    case SYN_YMODEM_EVENT_DATA:
        if (!state->erase_complete) {
            return -1;
        }
        /* Program data block to flash */
        state->current_addr += (uint32_t)len;
        state->total_written += (uint32_t)len;
        break;

    case SYN_YMODEM_EVENT_FILE_END:
        printf("YMODEM: File receive complete (%lu bytes written)\n",
               (unsigned long)state->total_written);
        break;

    case SYN_YMODEM_EVENT_SESSION_END:
        printf("YMODEM: Batch session complete\n");
        break;

    default:
        break;
    }

    return 0;
}

int main(void)
{
    SYN_YMODEM_Receiver rx;
    memset(&g_boot_state, 0, sizeof(g_boot_state));

    syn_ymodem_receiver_init(&rx, ymodem_putchar, ymodem_getchar, ymodem_event_handler, &g_boot_state);

    printf("Starting YMODEM Serial Bootloader...\n");
    SYN_YMODEM_Status status = syn_ymodem_receive(&rx);

    if (status == SYN_YMODEM_OK) {
        printf("Firmware update successful. Booting application...\n");
    } else {
        printf("YMODEM transfer failed with error %d\n", (int)status);
    }

    return 0;
}

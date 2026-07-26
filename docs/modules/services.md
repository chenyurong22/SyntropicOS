# System Services & Utility Modules

SyntropicOS provides high-level system services for interactive command line interfaces, software watchdog monitoring, logging, and time synchronization.

---

## 1. Interactive Serial CLI (`cli/syn_cli.h`)

The `syn_cli` module enables zero-allocation, interactive command-line interfaces over UART or USB Virtual COM Port.

### Features
- Command registry with argument parsing (`argc`, `argv`).
- Built-in `help` command automatically listing all registered commands and descriptions.
- Backspace, Line-feed (`\r\n`), and prompt string management (`"> "`).

### Complete Code Example

```c
#include <syntropic/cli/syn_cli.h>
#include <syntropic/port/syn_port_serial.h>

static SYN_CLI cli;

// Command 1: Echo handler
static int cmd_echo(int argc, char *argv[]) {
    if (argc < 2) {
        syn_cli_printf(&cli, "Usage: echo <text>\r\n");
        return 1;
    }
    syn_cli_printf(&cli, "Echo: %s\r\n", argv[1]);
    return 0;
}

// Command 2: System info handler
static int cmd_info(int argc, char *argv[]) {
    (void)argc; (void)argv;
    syn_cli_printf(&cli, "SyntropicOS v1.0.0\r\nUptime: %lu ms\r\n",
                   (unsigned long)syn_port_get_tick_ms());
    return 0;
}

static const SYN_CLI_Command command_table[] = {
    { "echo", "Echo argument back to terminal", cmd_echo },
    { "info", "Print system version and uptime", cmd_info },
};

void app_init(void) {
    syn_port_serial_init(115200);
    
    // Initialize CLI with command table and prompt
    syn_cli_init(&cli, command_table, sizeof(command_table)/sizeof(command_table[0]), "> ");
    syn_cli_printf(&cli, "\r\n--- SyntropicOS Serial CLI ---\r\n");
    syn_cli_print_prompt(&cli);
}

void app_loop(void) {
    uint8_t rx_char;
    if (syn_port_serial_read(&rx_char, 1) > 0) {
        // Feed character into CLI state machine
        syn_cli_process_char(&cli, (char)rx_char);
    }
}
```

---

## 2. Multi-Task Software Watchdog (`system/syn_watchdog.h`)

The software watchdog monitors multiple concurrent tasks, ensuring that if any individual protothread hangs or deadlocks, the system hardware watchdog resets the device.

```c
#include <syntropic/system/syn_watchdog.h>

static SYN_Watchdog wdt;

void task_worker(void) {
    // Kick task 0 heartbeat
    syn_watchdog_kick(&wdt, 0);
}

void app_init(void) {
    // Monitor 3 tasks with a 2000ms timeout window
    syn_watchdog_init(&wdt, 3, 2000);
}

void app_check_watchdog(void) {
    if (!syn_watchdog_check(&wdt)) {
        // One or more tasks failed to kick within 2000ms!
        printf("ERROR: Task deadlock detected! Resetting system...\n");
        syn_port_reset();
    }
}
```

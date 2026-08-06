# STM32F767 Production UDS Dual-Bank Flash Bootloader (FBL) Architecture Example

This example demonstrates the complete **ISO 14229-1 / AUTOSAR 17-Step 3-Phase Non-Volatile Server Memory Programming** specification on STM32F767 microcontrollers.

---

## 1. 17-Step 3-Phase ISO 14229-1 UDS Flashing Sequence

```text
===================================================================================
PHASE #1: PRE-PROGRAMMING STEP (Executed in Active Application `app/src/main.c`)
===================================================================================
 1. 0x10 0x03 : StartDiagnosticSessionControl (extendedSession)
 2. 0x85 0x02 : ControlDTCSetting (off)
 3. 0x28 0x03 : CommunicationControl (disableRxAndTx in application)
 4. 0x3E 0x80 : Functional TesterPresent keep-alive (suppressPosRspMsgIndicationBit)

===================================================================================
PHASE #2: PROGRAMMING STEP (Executed in Bootloader `bootloader/src/main.c`)
===================================================================================
 5. 0x10 0x02 : DiagnosticSessionControl (programmingSession) -> Jump to FBL
 6. 0x27 0x01 : SecurityAccess (requestSeed)
 7. 0x27 0x02 : SecurityAccess (sendKey) -> Unlock Security State
 8. 0x31 0x01 0xFF 0x00 : RoutineControl (eraseMemory via get_stm32f767_sector)
 9. 0x34      : RequestDownload (Target Memory Address & Size)
10. 0x36      : TransferData (Module Data Block Streaming #1, #2, #3...)
11. 0x37      : RequestTransferExit (Module Transfer Complete)
12. 0x31 0x01 0xFF 0x01 : RoutineControl (validate application / CRC32 Check)
13. 0x2E 0xF1 0x90      : WriteDataByIdentifier (VIN / Fingerprint)

===================================================================================
PHASE #3: POST-PROGRAMMING STEP (Executed after Reset in New Application)
===================================================================================
14. 0x11 0x01 : ECUReset (hardReset) -> Reset & Jump to Updated Application
15. 0x28 0x00 : CommunicationControl (enableRxAndTx in new application)
16. 0x85 0x01 : ControlDTCSetting (on)
17. 0x10 0x01 : DiagnosticSessionControl (defaultSession)
```

---

## 2. STM32F767 Flash Sector Organization (`get_stm32f767_sector`)

```c
uint32_t get_stm32f767_sector(uint32_t addr) {
    if (addr < 0x08008000U) return 0; /* Sector 0 (32 KB)  */
    if (addr < 0x08010000U) return 1; /* Sector 1 (32 KB)  */
    if (addr < 0x08018000U) return 2; /* Sector 2 (32 KB)  */
    if (addr < 0x08020000U) return 3; /* Sector 3 (32 KB)  */
    if (addr < 0x08040000U) return 4; /* Sector 4 (128 KB) */
    if (addr < 0x08080000U) return 5; /* Sector 5 (256 KB) */
    if (addr < 0x080C0000U) return 6; /* Sector 6 (256 KB) */
    return 7;                         /* Sector 7 (256 KB) */
}
```

---

## 3. Project Target Organization

- **`app/src/main.c`**: Active Application binary (Bank A `0x08020000` / Bank B `0x08100000`) embedding `SYN_FBL_AppHeader` and handling Pre-Programming Phase #1 & Post-Programming Phase #3.
- **`bootloader/src/main.c`**: FBL binary (Sector 0 `0x08000000`) implementing dynamic Bank A vs Bank B partition inspection, version comparison, staging bank targeted erase/write, and boot handover.

---

## 4. A/B Dual-Bank Partition Swap & Version Header (`SYN_FBL_AppHeader`)

```c
typedef struct __attribute__((packed)) {
    uint32_t magic;         /* 0x53594E31 ("SYN1") */
    uint16_t version_major; /* Major version (e.g. 1) */
    uint16_t version_minor; /* Minor version (e.g. 1) */
    uint16_t version_patch; /* Patch version (e.g. 0) */
    uint16_t reserved;
    uint32_t image_size;    /* Image size in bytes */
    uint32_t crc32;         /* Firmware image CRC32 checksum */
    uint8_t image_state;    /* 0x01: Valid, 0x02: Pending, 0xFF: Invalid */
    uint8_t padding[3];
} SYN_FBL_AppHeader;
```

---

## 5. UDS Service API Registration

- **0x10 DiagnosticSessionControl**: `syn_uds_set_session_transition_handler(server, cb, ctx)`
- **0x11 ECUReset**: `syn_uds_set_reset_handler(server, cb, ctx)`, `syn_uds_set_reset_wait_ms(server, wait_ms)`, `syn_uds_get_pending_reset(server)`, `syn_uds_clear_pending_reset(server)`
- **0x28 CommunicationControl**: `syn_uds_register_comm_control(server, handler, ctx)`
- **0x85 ControlDTCSetting**: `syn_uds_register_dtc(server, dtc, status, severity)`
- **0x3E TesterPresent**: `syn_uds_tick(server, dt_ms)`
- **0x2E WriteDataByIdentifier**: `syn_uds_register_did(server, did, data, len, writable)`

---

## 6. Post-Build Tooling Step & Header Patching (`tools/patch_header.py`)

In production CI/CD pipelines, calculating payload size and CRC32/ECDSA signatures occurs after compilation.

Run the provided post-build tool to patch `SYN_FBL_AppHeader` into `app.bin`:

```bash
python3 tools/patch_header.py app.bin 1 2 0
```

### Script Execution Flow
1. Reads `app.bin` binary file.
2. Calculates CRC32 checksum and payload byte count for bytes past `sizeof(SYN_FBL_AppHeader)`.
3. Packs magic `0x53594E31` ("SYN1"), target version numbers (`V1.2.0`), `crc32`, and `image_state` (`0x01` Valid).
4. Overwrites the 24-byte header block at offset 0 of `app.bin`.

---

## 7. Vector Table Relocation, Linker Script & CMake Build Integration

### Vector Table Relocation (`SCB->VTOR`)
When the bootloader hands over execution to Bank A (`0x08020000`) or Bank B (`0x08100000`), the application relocates `SCB->VTOR` at startup to prevent interrupt traps:

```c
volatile uint32_t *vtor = (volatile uint32_t *)0xE000ED08U;
*vtor = 0x08020000U; /* Bank A Base */
```

### Linker Script (`linker/stm32f767.ld`)
The linker script maps `.app_header` to the origin of the active application partition:

```ld
MEMORY {
    FLASH_FBL (rx)   : ORIGIN = 0x08000000, LENGTH = 128K
    FLASH_BANK_A (rx): ORIGIN = 0x08020000, LENGTH = 512K
    FLASH_BANK_B (rx): ORIGIN = 0x08100000, LENGTH = 512K
}
```

### CMake Automatic Post-Build Command (`CMakeLists.txt`)
Building the project automatically generates `app.bin` and patches `SYN_FBL_AppHeader` at offset 0:

```cmake
add_custom_command(TARGET stm32f7_app POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:stm32f7_app> app.bin
    COMMAND ${Python3_EXECUTABLE} tools/patch_header.py app.bin 1 1 0
)
```


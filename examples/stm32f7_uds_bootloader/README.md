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

- **`app/src/main.c`**: Active Application binary (Bank A `0x08020000`) handling Pre-Programming Phase #1 & Post-Programming Phase #3.
- **`bootloader/src/main.c`**: Minimal FBL binary (Sector 0 `0x08000000`) handling Programming Phase #2 and Flash sector erasing (`get_stm32f767_sector`).

---

## 4. UDS Service API Registration

- **0x10 DiagnosticSessionControl**: `syn_uds_set_session_transition_handler(server, cb, ctx)`
- **0x11 ECUReset**: `syn_uds_set_reset_handler(server, cb, ctx)`, `syn_uds_set_reset_wait_ms(server, wait_ms)`, `syn_uds_get_pending_reset(server)`, `syn_uds_clear_pending_reset(server)`
- **0x28 CommunicationControl**: `syn_uds_register_comm_control(server, handler, ctx)`
- **0x85 ControlDTCSetting**: `syn_uds_register_dtc(server, dtc, status, severity)`
- **0x3E TesterPresent**: `syn_uds_tick(server, dt_ms)`
- **0x2E WriteDataByIdentifier**: `syn_uds_register_did(server, did, data, len, writable)`


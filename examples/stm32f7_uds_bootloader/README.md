# STM32F767 Production UDS Dual-Bank Flash Bootloader (FBL) Architecture Example

This example demonstrates the complete **ISO 14229-1 / AUTOSAR 3-Phase Non-Volatile Server Memory Programming** specification on STM32F767 microcontrollers.

---

## 1. 3-Phase ISO 14229-1 UDS Flashing Sequence

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
 8. 0x31 0x01 0xFF 0x00 : RoutineControl (eraseMemory)
 9. 0x34      : RequestDownload (Target Memory Address & Size)
10. 0x36      : TransferData (Module Data Block Streaming #1, #2, #3...)
11. 0x37      : RequestTransferExit (Module Transfer Complete)
12. 0x31 0x01 0xFF 0x01 : RoutineControl (validate application / CRC32 Check)
13. 0x2E 0xF1 0x90      : WriteDataByIdentifier (VIN / Fingerprint)

===================================================================================
PHASE #3: POST-PROGRAMMING STEP
===================================================================================
14. 0x11 0x01 : ECUReset (hardReset) -> Reset & Jump to Updated Application
```

---

## 2. Project Target Organization

- **`app/src/main.c`**: Active Application binary (Bank A `0x08020000`) handling Pre-Programming Phase #1.
- **`bootloader/src/main.c`**: Minimal FBL binary (Sector 0 `0x08000000`) handling Programming Phase #2.

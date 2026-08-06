# STM32 ISO 14230-3 KWP2000 Diagnostic Server Example

This example demonstrates the **ISO 14230-3 Key Word Protocol 2000 (KWP2000)** over CAN ISO 15765-2 transport on STM32 microcontrollers.

---

## Supported Services

- **0x10 StartDiagnosticSession**: Standard (`0x81`), ECU Programming (`0x85`), Extended (`0x86`).
- **0x11 ECUReset**: Hard Reset (`0x01`), Soft Reset (`0x02`).
- **0x21 ReadDataByLocalIdentifier**: 1-byte Local Identifiers (LID e.g. `0x01` Engine RPM, `0x02` Coolant Temp).
- **0x22 ReadDataByCommonIdentifier**: 2-byte Common Identifiers (CID e.g. `0xF190` VIN).
- **0x27 SecurityAccess**: Seed (`0x01`) request & Key (`0x02`) unlock verification.
- **0x31 StartRoutineByLocalIdentifier**: RoutineControl execution.
- **0x34 / 0x36 / 0x37 Memory Transfer**: RequestDownload, TransferData streaming, RequestTransferExit.
- **0x3E TesterPresent**: S3 session keep-alive timer.

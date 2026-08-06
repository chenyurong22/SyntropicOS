# STM32 OCPP-J (Open Charge Point Protocol 1.6 / 2.0.1) EVSE Example

This example demonstrates the **Open Charge Point Protocol over JSON (OCPP-J 1.6 / 2.0.1 / 2.1)** for EVSE charging stations on STM32 microcontrollers.

---

## Key Features

- **Standard OCPP-J Framing**: JSON-formatted Call (`2`), CallResult (`3`), and CallError (`4`) frames over WebSocket (`syn_websocket`).
- **Charge Point Registration**: `BootNotification` payload formatting with vendor, model, serial number, and firmware version.
- **State Machine**: Connector status reporting (`Available`, `Preparing`, `Charging`, `SuspendedEV`, `SuspendedEVSE`, `Faulted`).
- **Authorization & Charging**: RFID `Authorize`, `StartTransaction`, `StopTransaction`, and periodic `MeterValues` (Wh, V, A, kW, SoC%).
- **Central System Remote Control**: `RemoteStartTransaction` and `RemoteStopTransaction` execution callbacks.

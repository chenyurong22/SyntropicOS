# STM32 HAL CAN ISO-TP (ISO 15765-2) Example

This example demonstrates how to integrate SyntropicOS ISO-TP (`syn_isotp.h`) with STM32 HAL drivers (`HAL_CAN_...`).

## Features
- Zero dynamic allocation (`malloc`).
- Full-Duplex ISO 15765-2:2016 multi-frame CAN segmentation and reassembly.
- Automatic Flow Control (FC) frame generation and STmin timer stepping.
- Direct HAL interrupt integration (`HAL_CAN_RxFifo0MsgPendingCallback`).


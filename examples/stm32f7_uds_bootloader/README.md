# STM32F767 Dual-Bank Application OTA & Minimal Bootloader Architecture Example

This example demonstrates the modern automotive **Dual-Bank Application OTA & Minimal Bootloader** architecture (aligned with ISO 26262, AIS-189, and AIS-190).

---

## 1. Project Structure (Two Independent Firmware Targets)

The example is split into two standalone, decoupled compilation targets:

```text
examples/stm32f7_uds_bootloader/
├── bootloader/
│   └── src/main.c     # Minimal Bootloader Binary (flashed to 0x08000000)
├── app/
│   └── src/main.c     # Active Application Binary with UDS OTA Engine (flashed to 0x08020000)
└── README.md          # Architecture & Linker Documentation
```

---

## 2. Architecture Overview

Rather than putting the heavy UDS protocol and network drivers into the Bootloader, the responsibility is cleanly split between two independent projects:

```text
  +-------------------------------------------------------------------------------+
  |  1. Minimal Bootloader Target (bootloader/src/main.c @ 0x08000000)            |
  |  - Zero UDS / Network Stack footprint (~4 KB total).                          |
  |  - Runs for ~5ms on power-on reset.                                           |
  |  - Reads syn_boot header, sets SCB->VTOR, and jumps to active Application.    |
  +-------------------------------------------------------------------------------+
                                        |
                                        v
  +-------------------------------------------------------------------------------+
  |  2. Active Application Target (app/src/main.c @ 0x08020000)                   |
  |  - Contains full UDS Stack (syn_uds, syn_isotp, syn_can).                     |
  |  - Services UDS 0x34 (RequestDownload) & 0x36 (TransferData) in background.    |
  |  - Writes incoming OTA firmware blocks into Inactive Staging Bank B (0x08100000).|
  |  - Verifies CRC32 (0x31), marks Bank B active in syn_boot, issues 0x11 reset. |
  +-------------------------------------------------------------------------------+
```

---

## 3. Flash Memory Map

| Partition | Target Binary | Start Address | End Address | Size | Function |
|---|---|---|---|---|---|
| **Minimal Bootloader** | `bootloader.elf` | `0x08000000` | `0x0801FFFF` | 128 KB | Minimal Bank Selector Bootloader (~4 KB used) |
| **Bank A (Active)** | `app.elf` | `0x08020000` | `0x080FFFFF` | 896 KB | Active Application Firmware (with UDS Stack) |
| **Bank B (Staging)** | `app_v2.elf` | `0x08100000` | `0x081FFFFF` | 1024 KB | Inactive OTA Staging Partition |

---

## 4. Linker Scripts (`stm32f767xx_bootloader.ld` vs `stm32f767xx_app.ld`)

### Bootloader Linker Script (`bootloader/stm32f767xx_bootloader.ld`)
```ld
MEMORY
{
  RAM (xrw)      : ORIGIN = 0x20020000, LENGTH = 384K
  FLASH_FBL (rx) : ORIGIN = 0x08000000, LENGTH = 128K
}
```

### Application Linker Script (`app/stm32f767xx_app.ld`)
```ld
MEMORY
{
  RAM (xrw)       : ORIGIN = 0x20020000, LENGTH = 384K
  FLASH_APP (rx)  : ORIGIN = 0x08020000, LENGTH = 896K
}
```

# Changelog

All notable changes to SyntropicOS will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Calendar Versioning](https://calver.org/) (`YYYY.M.MINOR`).

---

## [Unreleased]

### Added
- **ISO 14229-1 UDS Service Registrations & Handlers**: Extended `syn_uds` with service transition policies (`0x10`), deferred post-TX ECU reset handlers (`0x11`), CommunicationControl (`0x28`), ControlDTCSetting (`0x85`), TesterPresent S3 timer management (`0x3E`), and WriteDataByIdentifier (`0x2E` DID `0xF190`).
- **STM32F767 Production UDS Bootloader Example**: Added ISO 14229-1 / AUTOSAR 17-step 3-phase Non-Volatile Server Memory Programming architecture example with concrete STM32 HAL Flash erase and programming drivers (`examples/stm32f7_uds_bootloader`).
- **Documentation & Unit Tests**: Added zero-warning Doxygen annotations across `syn_cannm` and `syn_lintp`, and expanded unit test coverage in `test_workqueue.c`.

---

## [2026.7.4] - 2026-07-27

### Added
- **AUTOSAR CAN Network Management (`syn_cannm`)**: 5-state non-blocking FSM (`BUS_SLEEP`, `PREPARE_BUS_SLEEP`, `REPEAT_MESSAGE`, `NORMAL_OPERATION`, `READY_SLEEP`) with configurable timers and zero dynamic memory allocation.

---

## [2026.7.3] - 2026-07-27

### Added
- **CJ/T 188 Protocol Engine (`syn_cjt188`)**: Smart metering protocol driver for water, heat, and gas meters.
- **DL/T 645 Protocol Engine (`syn_dlt645`)**: Electricity meter protocol implementation (1997 and 2007 standards).
- **M-Bus Protocol Engine (`syn_mbus`)**: EN 13757-2/3 master and slave communication stack.
- **Enhanced Button Controller (`syn_button`)**: Support for multi-click timing windows, hold auto-repeat, and multi-button combination event handlers (`SYN_ButtonCombo`).
- **Static Analysis & Sanitizer Infrastructure**: Integrated GCC `-fanalyzer`, ThreadSanitizer (TSan), Flawfinder, and containerized Valgrind Memcheck.

### Fixed
- Enforced production NULL and parameter validation guards across 70+ core, driver, net, storage, and protocol modules.
- Resolved frame length validation and buffer overflow bounds checks in Modbus, DLT645, CBOR, and COBS parsers.

---

## [2026.7.2] - 2026-07-25

### Added
- **EtherCAT Protocol Engine (`syn_ethercat`)**: Zero-allocation EtherCAT slave protocol stack integrated with OpenEtherCATSociety SOES.
- **PlatformIO Integration**: Official `library.json` manifest, top-level wrapper header, and automated CI publishing pipeline.
- **AES-128 FIPS 197 Cipher (`syn_aes128`)**: Constant-time AES-128 encryption/decryption with CBC mode and PKCS#7 padding.
- **Dual-Bank NVS Settings Storage (`syn_settings`)**: Transactional dual-bank non-volatile settings storage with wear-leveling and VFS export/import.
- **3D Jerk-Limited S-Curve Generator (`syn_scurve3d`)**: Synchronized 3-axis motion trajectory planner.
- **MISRA C:2023 Compliance & Renode Emulation**: Containerized MISRA C:2023 scanner and Renode board-level hardware emulation suite.

---

## [2026.7.1] - 2026-07-24

### Added
- **ISO 15765-2 (ISO-TP) Transport Stack (`syn_isotp`)**: Multi-frame CAN transport layer with block size (BS) and separation time (STmin) flow control, plus opt-in CAN FD support.
- **LIN Bus Protocol Stack (`syn_lin`, `syn_lintp`)**: LIN 2.1 / 2.2a master/slave protocol stack and LIN diagnostic transport layer.
- **USITT DMX512-A Protocol Engine (`syn_dmx512`)**: 512-channel lighting control master and slave protocol stack.
- **High-Precision Clock & Time Sync (`syn_hpclock`, `syn_timesync`)**: 64-bit system-clock precision timestamping primitive and PPS-disciplined time service.
- **Geodetic Coordinate Library (`syn_geo`)**: WGS84 ellipsoid conversions, Haversine/Vincenty distance calculations, and 3D local ENU transformations.
- **DALI Lighting Protocol Stack (`syn_dali`)**: IEC 62386 control gear and controller implementation.
- **NMEA 2000 & SAE J1939 CAN Stacks (`syn_n2k`, `syn_j1939`)**: Marine and heavy-duty vehicle CAN protocol engines with full PGN decoding.
- **O(1) Hashed Timing Wheel & Packet Pool**: Zero-copy packet buffer pool (`syn_netbuf`) and O(1) hashed timing wheel timer scheduler.

---

## [2026.7.0] - 2026-07-23

### Added
- **WireGuard VPN Client (`syn_wg`)**: Bare-metal WireGuard protocol implementation using Noise_IK handshake, ChaCha20-Poly1305, Curve25519, and BLAKE2s.
- **CANopen DS301 & CiA 402 Servo Profile (`syn_canopen`, `syn_cia402`)**: Complete CANopen slave engine with SDO/PDO management, Heartbeat, LSS (CiA 305), and CiA 402 motor drive FSM.
- **Sensorless FOC Motor Control (`syn_foc`)**: Field-Oriented Control with Park/Clarke transforms, Space Vector PWM (SVPWM), and Sliding Mode Observer (SMO) speed estimation.
- **Dynamic Biquad Filter Generator (`syn_filter`)**: Fixed-point IIR filter design generator for low-pass, high-pass, band-pass, and notch filtering.
- **Mahony 6-DOF AHRS Sensor Fusion (`syn_sensor`)**: Quaternion IMU orientation estimation filter.
- **Multi-Axis Interpolator (`syn_interpolator`)**: Linear, circular, and Bezier path motion planning primitives.
- **Event-Driven Task Blocking (`syn_sched`)**: Added `PT_BLOCK_EVENT`, `PT_DEFER`, and per-priority round-robin scheduling primitives.

---

## [2026.6.2] - 2026-06-30

### Added
- **AMP Multicore Support (`syn_multicore`)**: Symmetric/Asymmetric multiprocessing support with hardware spinlocks, execution barriers, and cross-core mailbox messaging (RP2040 support).
- **Crypto & Security Manager (`syn_sha256`, `syn_hkdf`, `syn_fwupdate`)**: SHA-256/HMAC digest engine, HKDF key derivation, and signed firmware verification.
- **Memory Pool Allocator (`syn_pool`)**: Fixed-block zero-fragmentation memory pool manager.
- **Core Dump & Diagnostic Logger (`syn_coredump`, `syn_errlog`)**: Non-volatile panic state persistence and fault analysis system.
- **Tickless Power Management (`syn_tickless`, `syn_power`)**: Low-power sleep mode integration and dynamic hardware timer suppression.

---

## [2026.6.1] - 2026-06-29

### Added
- **Initial Release of SyntropicOS**: Zero-heap cooperative embedded operating system and protocol framework for microcontrollers.
- **Core Cooperative Scheduler (`syn_sched`)**: Lightweight protothread engine based on Duff's device state machines.
- **Peripheral Drivers & Utilities**: Modular drivers for GPIO, UART, ADC, I2C, SPI, CAN, PWM, Timers, Ring Buffers, and Finite State Machines.
- **Arduino Library Support**: Packaged structure for single-header inclusion in Arduino IDE and PlatformIO environments.

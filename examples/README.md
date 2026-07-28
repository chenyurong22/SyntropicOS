# SyntropicOS Examples Directory

This directory contains bare-metal C and MCU HAL example projects demonstrating SyntropicOS non-blocking drivers, protocol stacks, and coroutine scheduling.

## Categories & Examples

### Industrial Protocols & Lighting
- **[`stm32_modbus_tcp`](stm32_modbus_tcp)** — Dual Modbus TCP Server (port 502) & Client (Master) in a single project.
- **[`stm32_modbus_master`](stm32_modbus_master)** — STM32 RS485 Modbus RTU Master querying field slave registers.
- **[`stm32_modbus_slave`](stm32_modbus_slave)** — STM32 RS485 Modbus RTU Slave register map & exception handler.
- **[`ModbusSlave`](ModbusSlave)** — Generic bare-metal Modbus RTU Slave implementation.
- **[`stm32_ethercat_servo`](stm32_ethercat_servo)** — EtherCAT (IEEE 802.3 EtherType 0x88A4) Slave node & CiA 402 drive control.
- **[`stm32_bacnet_mstp`](stm32_bacnet_mstp)** — STM32 RS485 BACnet MS/TP Smart Thermostat / HVAC Sensor node.
- **[`stm32_dali_lighting`](stm32_dali_lighting)** — STM32 DALI (IEC 62386) LED Dimmer / Control Gear node.
- **[`stm32_dmx512`](stm32_dmx512)** — STM32 DMX512 stage lighting receiver & PWM output controller.

### Automotive & Marine Fieldbus
- **[`stm32_canopen`](stm32_canopen)** — STM32 CANopen (CiA 301) Node with SDO/PDO dictionary maps.
- **[`stm32_can_isotp`](stm32_can_isotp)** — STM32 ISO-TP (ISO 15765-2) CAN multi-frame transport layer.
- **[`stm32_j1939`](stm32_j1939)** — SAE J1939 Heavy-Duty Vehicle CAN protocol & DM1 diagnostics.
- **[`stm32_lin_bus`](stm32_lin_bus)** — LIN 2.1 Automotive Single-Wire Bus Master & Slave state machine.
- **[`stm32_nmea2k`](stm32_nmea2k)** — NMEA 2000 (IEC 61162-3) Marine CAN PGN encoder & decoder.

### Smart Energy & Power Management
- **[`stm32_mbus_meter`](stm32_mbus_meter)** — M-Bus (Meter-Bus EN 13757) Utility Meter Reader (Water, Gas, Heat).
- **[`stm32_pmbus_power`](stm32_pmbus_power)** — PMBus 1.2/1.3 Digital Power Supply Telemetry & Linear11/16 format decoding.
- **[`stm32_smbus_battery`](stm32_smbus_battery)** — SMBus 2.0 / SBS 1.1 Smart Battery System Telemetry & Alert Handler.
- **[`PmbusTelemetry`](PmbusTelemetry)** — Generic PMBus power telemetry converter.
- **[`stm32_dlt645_meter`](stm32_dlt645_meter)** — DLT645 Smart Electricity Meter protocol parser.

### Microcontroller Peripheral HAL & CLI Shell
- **[`stm32_cli_shell`](stm32_cli_shell)** — Interactive USART CLI Shell (`led`, `status`, `temp`).
- **[`SerialCLI`](SerialCLI)** — Generic serial command-line interpreter over UART.
- **[`stm32_encoder_button`](stm32_encoder_button)** — EC11 Rotary Encoder & push-button debounced menu controller.
- **[`stm32_joystick`](stm32_joystick)** — Dual-axis analog joystick ADC sampler & 8-way D-Pad decoder.
- **[`stm32_keypad`](stm32_keypad)** — 4x4 Matrix keypad scanner & PIN security entry.
- **[`stm32_touch_key`](stm32_touch_key)** — 4-channel capacitive touch sensing key pad & baseline calibration.
- **[`stm32_dipswitch`](stm32_dipswitch)** — 8-position DIP switch address reader & baud rate selector.
- **[`stm32_soft_pwm`](stm32_soft_pwm)** — Multi-channel software PWM LED dimmer & motor driver.
- **[`stm32_led`](stm32_led)** — GPIO status LED heartbeat, blinking, and error patterns.
- **[`stm32_smart_led`](stm32_smart_led)** — WS2812B / Neopixel Smart RGB LED strip rainbow animator.
- **[`stm32_buzzer`](stm32_buzzer)** — Piezo buzzer audio tone, chime arpeggio, and siren alarm.
- **[`stm32_button`](stm32_button)** — Multi-tap button gesture & combo handler.








- **[`stm32_spsc_usart`](stm32_spsc_usart)** — Single-Producer Single-Consumer lock-free ring queue for USART RX ISR.
- **[`stm32_ringbuf_usart`](stm32_ringbuf_usart)** — Non-blocking ring buffer USART RX ingestion.
- **[`stm32_uart_mcu_comm`](stm32_uart_mcu_comm)** — Inter-MCU UART packet routing & COBS framing.
- **[`stm32_crypto_usart`](stm32_crypto_usart)** — Encrypted USART receiver with SHA-256 digest & AES-128.
- **[`stm32_flash`](stm32_flash)** — Internal MCU flash erase, sector write, and parameter store.
- **[`stm32_profiler`](stm32_profiler)** — CPU usage & execution time task profiler (`syn_profiler`).
- **[`stm32_log_console`](stm32_log_console)** — Asynchronous ring-buffered log console.
- **[`stm32_serial`](stm32_serial)** — Bare-metal serial UART transmit & receive.
- **[`stm32_json`](stm32_json)** — Zero-malloc JSON parsing & encoding.
- **[`stm32_ir_remote`](stm32_ir_remote)** — NEC protocol Infrared Remote Control decoder.
- **[`pico_dual_core`](pico_dual_core)** / **[`PicoDualCore`](PicoDualCore)** — RP2040 SMP dual-core cooperative task execution.
- **[`PicoBlink`](PicoBlink)** — Raspberry Pi Pico bare-metal GPIO blink.
- **[`esp32_ota`](esp32_ota)** — ESP32 Firmware Over-The-Air (OTA) update task.

### DSP & Motion Control
- **[`stm32_stepper`](stm32_stepper)** — Stepper motor trapezoidal speed ramp & position controller.
- **[`MotionPlanner`](MotionPlanner)** — Trapezoidal S-curve motor motion planner.
- **[`MotorFSM`](MotorFSM)** — Finite state machine controlling a DC motor ramp profile.
- **[`PID_TempControl`](PID_TempControl)** — Closed-loop integer PID temperature controller.
- **[`BiquadFilter`](BiquadFilter)** — Audio & sensor digital biquad IIR filtering.
- **[`FftSpectrumAnalyzer`](FftSpectrumAnalyzer)** — Real-time FFT spectral decomposition.


### IoT & Network Protocol Stacks
- **[`MqttClient`](MqttClient)** — MQTT v3.1.1 network client with QoS0/QoS1 support.
- **[`CoapClient`](CoapClient)** — CoAP (RFC 7252) UDP client with Option header encoding.
- **[`WebsocketServer`](WebsocketServer)** — Lightweight WebSocket server handling frames & handshakes.
- **[`EthernetWebServer`](EthernetWebServer)** — Embedded HTTP 1.1 Web Server.
- **[`HttpClient`](HttpClient)** — Non-blocking HTTP GET/POST client.
- **[`DnsResolver`](DnsResolver)** — DNS hostname resolution client.
- **[`SntpClock`](SntpClock)** — SNTP Network Time Protocol client sync.
- **[`Telemetry_CBOR`](Telemetry_CBOR)** — Concise Binary Object Representation (CBOR RFC 8949) encoder.
- **[`stm32_sim800_mqtt`](stm32_sim800_mqtt)** — SIM800 GSM/GPRS Cellular Modem MQTT Client.

### System Core & Utilities
- **[`Blink`](Blink)** — Basic protothread LED blinker.
- **[`ButtonEvents`](ButtonEvents)** — Tap gestures, double clicks, and chorded button combos.
- **[`SensorLogger`](SensorLogger)** — Dual-channel ADC sampling, EMA filtering, and Serial CLI.
- **[`PersistentSettings`](PersistentSettings)** — Wear-leveled key-value parameter storage.
- **[`SysMonitor`](SysMonitor)** — Task execution monitor & health logger.
- **[`TaskMailbox`](TaskMailbox)** — Inter-task message passing via Mailbox queue.
- **[`TaskWatchdog`](TaskWatchdog)** — Hardware & software multi-task watchdog supervisor.
- **[`GpsNmeaParser`](GpsNmeaParser)** — NMEA 0183 GPS sentence stream parser.

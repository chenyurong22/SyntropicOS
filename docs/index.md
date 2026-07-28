# SyntropicOS Documentation

**High-Performance Bare-Metal Application Framework & Cooperative OS**

[![GitHub Repository](https://img.shields.io/badge/GitHub-outlookhazy%2FSyntropicOS-blue?logo=github)](https://github.com/outlookhazy/SyntropicOS)
[![Latest Release](https://img.shields.io/github/v/release/outlookhazy/SyntropicOS?logo=github)](https://github.com/outlookhazy/SyntropicOS/releases)
[![License](https://img.shields.io/github/license/outlookhazy/SyntropicOS)](https://github.com/outlookhazy/SyntropicOS/blob/main/LICENSE)

SyntropicOS is a zero-overhead, production-grade C99 framework designed for deeply embedded systems. It provides stackless multitasking, non-blocking drivers, industrial fieldbuses, and display graphics for targets ranging from 8-bit microcontrollers to 32-bit Cortex-M and RISC-V targets.

---

## Technical Specifications At-a-Glance

| Feature | Design Specification |
|---|---|
| **Concurrency** | Cooperative protothreads (`syn_pt`). Continuation state costs **2 bytes RAM** per thread. |
| **Task Scheduler** | Cooperative task runner (`syn_sched`). Task descriptors cost **~16–28 bytes RAM** per task. |
| **Memory Allocation** | **100% Zero-Heap / Static Allocation**. No `malloc()` or dynamic pool fragmentation over long runtimes. |
| **Execution Model** | All 70+ drivers & protocol stacks are written as **non-blocking state machines**. |
| **Compatibility** | Standard **C99**. Compiles with GCC, Clang, IAR, Keil, STM32CubeIDE, and Arduino IDE. |

---

## Module Documentation Index

Quick-jump to specific feature guides and API references:

### ⚡ Core & Multitasking ([Read Core Docs →](modules/multitasking.md))
- **[Protothreads (`syn_pt`)](modules/multitasking.md#1-protothreads)**: Stackless coroutines for non-blocking task execution.
- **[Task Scheduler (`syn_sched`)](modules/multitasking.md#2-cooperative-scheduler)**: Cooperative task runner with priority & delay timers.
- **[Active Objects (`syn_ao`)](modules/integration.md#2-active-object-pattern)**: FSM state machine + SPSC queue + task runner actor model.
- **[Event Flags & Mailboxes](modules/core.md)**: Thread-safe inter-task messaging and synchronization.

### 🎛️ Input / Output Drivers ([Read I/O Docs →](modules/io.md))
- **[Buttons (`syn_button`)](modules/io.md#1-button-driver)**: Debounced buttons, multi-click tap gestures, long-press, auto-repeat, and combos.
- **[Rotary Encoder (`syn_encoder`)](modules/io.md#2-rotary-encoder-driver)**: Quadrature rotary decoding and velocity tracking.
- **[LED Controller (`syn_led`)](modules/io.md#3-non-blocking-led-driver)**: Pattern blinking, flash sequences, and Morse sequences.
- **[Software PWM (`syn_soft_pwm`)](modules/io.md#4-software-pwm-driver)**: Timerless PWM generation on arbitrary GPIO pins.

### 📡 Communications & Protocol Stacks ([Read Comm Docs →](modules/communication.md))
- **[COBS Framing (`syn_cobs`)](modules/communication.md#1-cobs--packet-router-pipeline)**: Zero-overhead `0x00`-delimited packet framing.
- **[Packet Router (`syn_router`)](modules/communication.md#1-cobs--packet-router-pipeline)**: Addressed packet dispatch (Master/Slave Node IDs) with ACKs.
- **[Industrial Modbus (`syn_modbus`)](modules/communication.md)**: Modbus RTU & Modbus TCP Master/Slave stacks.
- **[Building Automation (`syn_bacnet` / `syn_dali`)](modules/communication.md)**: BACnet MS/TP (ISO 16484-5) & DALI Lighting (IEC 62386) protocol engines.
- **[M-Bus Metering (`syn_mbus`)](modules/communication.md#2-m-bus-protocol)**: EN 13757 European utility meter bus decoder.
- **[Automotive ISO-TP & J1939](modules/communication.md)**: CAN bus multi-frame transport and heavy vehicle PGN/SPN decoder.


### 💾 Storage & Filesystems ([Read Storage Docs →](modules/storage.md))
- **[Persistent Settings (`syn_settings`)](modules/storage.md#1-persistent-settings-manager)**: Wear-leveled flash configuration with load-or-default & CRC-16.
- **[Virtual File System (`syn_vfs`)](modules/storage.md#2-virtual-file-system)**: POSIX-like VFS abstraction for LittleFS and FAT.

### 🖥️ Display & Embedded UI ([Read Display Docs →](modules/display.md))
- **[Display Canvas (`syn_canvas`)](modules/display.md#1-framebuffer-display-canvas)**: Hardware-independent 1bpp/16bpp framebuffer & 2D graphics.
- **[Immediate-Mode GUI (`syn_imgui`)](modules/display.md#2-immediate-mode-gui)**: Zero-heap UI widgets (buttons, sliders, gauges, graphs).

### 📈 DSP & TinyML Neural Networks ([Read DSP & TinyML Docs →](modules/dsp.md))
- **[Fixed-Point Filters (`syn_filter`)](modules/dsp.md#1-digital-filters)**: Biquad lowpass/highpass, EMA, and median spike rejection.
- **[Spectral Analysis (`syn_fft` / `syn_dsp`)](modules/dsp.md#3-fast-fourier-transform--peak-detection)**: Radix-2 FFT, DCT-II, windowing, and peak tracking.
- **[TinyML Neural Networks (`syn_nn`)](modules/dsp.md#4-tinyml--fixed-point-neural-networks-utilsyn_nnh)**: Quantized 1D-CNNs, 1D Pooling, Dense layers, Self-Attention, and Protothread inference.

### 🔬 Diagnostics & System Services ([Read Debug Docs →](modules/debug.md))
- **[Lightweight Event Tracer (`syn_trace`)](modules/debug.md#1-lightweight-event-tracer)**: Timestamped circular event recorder for ISRs & tasks.
- **[Task CPU Profiler (`syn_profiler`)](modules/debug.md#2-task-cpu-profiler)**: Task CPU percentage, peak execution time, and run metrics.
- **[Serial CLI (`syn_cli`)](modules/services.md#1-interactive-serial-cli)**: Zero-allocation interactive shell with command auto-help.
- **[Software Watchdog (`syn_watchdog`)](modules/services.md#2-multi-task-software-watchdog)**: Multi-task heartbeat monitor and deadlock prevention.

---

## Getting Started & Platform Guides

- **[Getting Started Guide](getting-started.md)** — Step-by-step setup for C99 CMake & Makefile projects.
- **[Arduino Compatibility Guide](arduino.md)** — Installing via Library Manager and working with Multi-Tab sketch examples.
- **[Porting & System Integration](porting-guide.md)** — Implementing custom GPIO, UART, and timer tick ports.

# Directory Structure

```text
SyntropicOS/                          ← SyntropicOS repository
├── library.properties         ← Arduino Library Manager metadata
├── CMakeLists.txt             ← CMake build system
├── Makefile                   ← Top-level Makefile for building & testing
├── sources.mk                 ← Include file for external Makefiles
├── Doxyfile                   ← Doxygen API documentation configuration
├── mkdocs.yml                 ← MkDocs documentation site configuration
├── src/                       ← Core C99 OS & driver source files
│   ├── syntropic/
│   │   ├── syntropic.h             ← Umbrella header
│   │   ├── syn_config_template.h   ← System configuration template
│   │   ├── common/                ← Types, error codes, compiler macros
│   │   ├── port/                  ← Platform port interface headers (HAL)
│   │   ├── port_stubs/            ← Weak fallback stubs
│   │   ├── drivers/               ← Peripherals (GPIO, UART, ADC, I2C, SPI)
│   │   ├── pt/                    ← Protothread coroutines, semaphores
│   │   ├── sched/                 ← Scheduler, timers, workqueues, mailboxes
│   │   ├── log/                   ← Logging and data logging
│   │   ├── cli/                   ← Command-line interface
│   │   ├── util/                  ← Fixed-point math, matrices, CRC, COBS, ring buffer
│   │   ├── input/                 ← Button debouncing, rotary encoders
│   │   ├── output/                ← LED control, soft PWM
│   │   ├── display/               ← Framebuffer canvas & drawing primitives
│   │   ├── ui/                    ← Zero-allocation IMGUI framework & menus
│   │   ├── control/               ← PID controller, auto-tuner
│   │   ├── motor/                 ← Motor control, stepper, servo, FOC
│   │   ├── dsp/                   ← Digital filters, FFT, Kalman filter
│   │   ├── proto/                 ← CANopen, CiA 402, Modbus RTU/TCP, IR
│   │   ├── net/                   ← Network stack (HTTP, WS, MQTT, DNS, SNTP, WireGuard)
│   │   ├── sensor/                ← Sensor polling framework
│   │   ├── storage/               ← Parameter store, VFS, LittleFS
│   │   └── system/                ← Boot manager, coredump, power manager
│   └── port/                      ← Native port implementations
│       ├── posix/                 ← POSIX socket & timer port
│       ├── stm32f4/               ← STM32F4 bare-metal
│       ├── stm32_hal/             ← STM32 HAL
│       ├── esp32/                 ← ESP-IDF
│       ├── rp2040/                ← Raspberry Pi Pico SDK
│       └── arduino/               ← Arduino C++ SDK
├── tests/                     ← Comprehensive test suites
│   ├── unit/                  ← Unity unit test framework & unit test drivers
│   │   ├── mocks/             ← Weak hardware port mocks
│   │   └── unity/             ← Unity test engine
│   ├── integration/           ← 3rd-party containerized integration suite
│   │   ├── docker-compose.yml ← Orchestration for 8 production daemons
│   │   ├── run_integration.sh ← Integration test driver runner
│   │   ├── services/          ← Service definitions (Nginx, CoreDNS, Mosquitto, etc.)
│   │   └── test_*_integration.c
│   ├── qemu/                  ← QEMU bare-metal Cortex-M4 boot emulation
│   ├── fuzz/                  ← LLVM libFuzzer protocol fuzzing targets
│   ├── sim/                   ← Physical motor & plant simulation harness
│   └── imgui_host/            ← Host GUI screenshot & timeline generator
├── tools/                     ← Containerization & CI automation tooling
│   └── containers/
│       ├── Containerfile      * Comprehensive Docker/Podman build image
│       └── Makefile           * Container execution targets
├── examples/                  ← Hardware & SDK example projects
└── docs/                      ← Project documentation
```

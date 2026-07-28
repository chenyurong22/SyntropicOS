# IDE Integration & Setup Guides

Step-by-step setup guides for integrating SyntropicOS into standard microcontroller IDEs and toolchains: STM32CubeIDE, VS Code (PlatformIO & CMake), Keil MDK-ARM, IAR Embedded Workbench, and Arduino IDE.

---

## 1. STM32CubeIDE

### Project Setup
1. **Add SyntropicOS Source Tree**:
   - Copy or add a git submodule of `SyntropicOS` into your STM32CubeIDE project root folder (e.g. `Middlewares/Third_Party/SyntropicOS`).
2. **Configure Include Paths**:
   - Right-click your project $\rightarrow$ **Properties** $\rightarrow$ **C/C++ Build** $\rightarrow$ **Settings** $\rightarrow$ **MCU GCC Compiler** $\rightarrow$ **Include paths**.
   - Add `${workspace_loc:/${ProjName}/Middlewares/Third_Party/SyntropicOS/src}`.
   - Ensure `${workspace_loc:/${ProjName}/Core/Inc}` is also in the include path (where your `syn_config.h` will reside).
3. **Add Source Folders to Build**:
   - Right-click `Middlewares/Third_Party/SyntropicOS/src/syntropic` $\rightarrow$ **Source Actions** $\rightarrow$ **Add to Build** (or ensure it is not marked as *Exclude from build*).
   - If using STM32 HAL porting, also include `src/port/stm32_hal/port_stm32_hal.c`.
4. **Set C Standard**:
   - In **MCU GCC Compiler** $\rightarrow$ **Miscellaneous**, set C Language Standard to `-std=c99`.
5. **Configuration Header**:
   - Copy `src/syntropic/common/syn_config_template.h` to your project's `Core/Inc/syn_config.h`.

---

## 2. VS Code (PlatformIO & CMake + Cortex-Debug)

### PlatformIO Integration
SyntropicOS is a native PlatformIO library (`library.json`). Include it directly via `lib_deps` in your `platformio.ini`:

```ini
[env:stm32f407]
platform = ststm32
board = disco_f407vg
framework = stm32cube
build_flags =
    -std=c99
lib_deps =
    SyntropicOS
```

Or reference the Git repository directly:

```ini
lib_deps =
    https://github.com/outlookhazy/SyntropicOS.git
```

### CMake + VS Code Cortex-Debug
1. **`CMakeLists.txt`**:
   ```cmake
   cmake_minimum_required(VERSION 3.20)
   project(my_embedded_app C)
   set(CMAKE_C_STANDARD 99)

   add_subdirectory(lib/SyntropicOS)
   add_executable(my_app src/main.c)
   target_link_libraries(my_app PRIVATE syntropic)
   ```

2. **`.vscode/launch.json`** (Cortex-Debug ST-Link):
   ```json
   {
     "version": "0.2.0",
     "configurations": [
       {
         "name": "Cortex-Debug (ST-Link)",
         "type": "cortex-debug",
         "request": "launch",
         "servertype": "openocd",
         "executable": "${workspaceRoot}/build/my_app.elf",
         "device": "STM32F407VG",
         "configFiles": [
           "interface/stlink.cfg",
           "target/stm32f4x.cfg"
         ]
       }
     ]
   }
   ```

---

## 3. Keil MDK-ARM (uVision)

### Project Setup
1. **Add Group & Files**:
   - In the Project Workspace tree, right-click target $\rightarrow$ **Add Group...** $\rightarrow$ Name it `SyntropicOS`.
   - Right-click `SyntropicOS` group $\rightarrow$ **Add Existing Files to Group 'SyntropicOS'**.
   - Select all `.c` files in `src/syntropic/` subdirectories (`common`, `kernel`, `drivers`, `input`, `output`, `protocol`, `system`).
2. **Set Include Paths**:
   - Click **Options for Target** (Alt+F7) $\rightarrow$ **C/C++** tab.
   - In **Include Paths**, add:
     `..\SyntropicOS\src;`
     `..\Core\Inc;`
3. **Compiler Options**:
   - Check **C99 Mode** in the C/C++ tab.
   - Add preprocessor define `SYN_USE_CONFIG_HEADER=1`.
4. **SysTick & Critical Sections**:
   - Ensure `SysTick_Handler` in `stm32f4xx_it.c` increments `HAL_IncTick()` or `syn_port_get_tick_ms()`.

---

## 4. IAR Embedded Workbench for ARM

### Project Setup
1. **Add Project Group**:
   - Right-click project root in Workspace window $\rightarrow$ **Add** $\rightarrow$ **Add Group...** $\rightarrow$ Name it `SyntropicOS`.
   - Right-click `SyntropicOS` $\rightarrow$ **Add** $\rightarrow$ **Add Files...**
   - Select all `.c` files from `src/syntropic/`.
2. **Preprocessor Options**:
   - Right-click project $\rightarrow$ **Options** $\rightarrow$ **C/C++ Compiler** $\rightarrow$ **Preprocessor**.
   - Add to Additional include directories:
     `$PROJ_DIR$/../SyntropicOS/src`
     `$PROJ_DIR$/../Core/Inc`
3. **Language Standards**:
   - In **C/C++ Compiler** $\rightarrow$ **Language 1** tab, select **C Dialect** $\rightarrow$ **C99**.

---

## 5. Arduino IDE (v1.8 / v2.x)

### Library Installation
SyntropicOS is packaged as a standard Arduino Library (`library.properties`).

1. **Installation Methods**:
   - **Library Manager**: Search for `SyntropicOS` in **Sketch** $\rightarrow$ **Include Library** $\rightarrow$ **Manage Libraries...**
   - **Add .ZIP Library**: Download repository release ZIP $\rightarrow$ **Sketch** $\rightarrow$ **Include Library** $\rightarrow$ **Add .ZIP Library...**
   - **Manual Placement**: Extract into your sketchbook `libraries/` directory (`Arduino/libraries/SyntropicOS`).

2. **Header Inclusion & Sketch Usage**:
   - Include `<syntropic/syntropic.h>` in your `.ino` sketch:
     ```cpp
     #include <syntropic/syntropic.h>

     void setup() {
         syn_gpio_init(LED_BUILTIN, SYN_GPIO_OUTPUT);
     }

     void loop() {
         syn_gpio_toggle(LED_BUILTIN);
         delay(500);
     }
     ```


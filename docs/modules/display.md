# Display & UI Modules

SyntropicOS provides hardware-independent framebuffer drawing (`syn_canvas`), 2D graphics primitives (`syn_gfx`), and a zero-allocation immediate-mode GUI engine (`syn_imgui`).

---

## 1. Framebuffer Display Canvas (`display/syn_canvas.h`)

The `syn_canvas` driver provides a hardware-agnostic framebuffer supporting both **1bpp (Monochrome OLED)** and **16bpp (RGB565 TFT)** displays.

### Architecture Data Flow

```mermaid
flowchart LR
    Drawing["Drawing Primitives (line, text, rect)"] --> Canvas["SYN_Canvas Framebuffer"]
    Canvas -- syn_canvas_flush() --> HardwareFn["Flush Function (SPI/I2C DMA)"]
    HardwareFn --> OLED["Physical OLED / TFT Display"]
```

### Complete Code Example (SSD1306 Mono OLED)

```c
#include <syntropic/display/syn_canvas.h>

// Monochrome OLED (128×64 pixels, 1bpp = 1024 bytes)
static uint8_t framebuf[128 * 64 / 8];
static SYN_Canvas canvas;

// Flush callback: sends raw framebuffer bytes to display driver over I2C/SPI
static void oled_flush_callback(const uint8_t *buf, size_t len, void *ctx) {
    // Send 1024-byte framebuffer via I2C/SPI DMA
    SSD1306_Transmit_Buffer(buf, len);
}

void display_setup(void) {
    // Initialize canvas: 128px width, 64px height, 1bpp monochrome format
    syn_canvas_init(&canvas, framebuf, 128, 64, 1, oled_flush_callback, NULL);
}

void render_dashboard(int temperature, int battery_pct) {
    char str[32];
    
    // Clear previous frame
    syn_canvas_clear(&canvas);

    // Draw header text using built-in 5x7 font
    syn_canvas_text(&canvas, 0, 0, "SyntropicOS Dashboard");
    
    // Draw horizontal dividing line
    syn_canvas_line(&canvas, 0, 10, 127, 10);

    // Draw metric labels
    snprintf(str, sizeof(str), "Temp: %d C", temperature);
    syn_canvas_text(&canvas, 0, 20, str);

    snprintf(str, sizeof(str), "Batt: %d%%", battery_pct);
    syn_canvas_text(&canvas, 0, 32, str);

    // Draw battery level progress bar outline & fill
    syn_canvas_rect(&canvas, 70, 32, 50, 8); // Outline
    syn_canvas_fill_rect(&canvas, 71, 33, (battery_pct * 48) / 100, 6); // Fill

    // Push framebuffer to hardware
    syn_canvas_flush(&canvas);
}
```

---

## 2. Immediate-Mode GUI (`ui/syn_imgui.h`)

The `syn_imgui` module provides a zero-allocation Immediate-Mode GUI framework. Widgets are drawn and input-tested in a single pass each frame without maintaining complex DOM trees or allocating dynamic heap memory.

### Features
- Widgets: Buttons, Sliders, Checkboxes, Progress Bars, Gauges, Real-time Graphs.
- Navigation: Rotary Encoder step navigation, Button clicks, or Touchscreen X/Y coordinates.

### Complete IMGUI Menu Example

```c
#include <syntropic/ui/syn_imgui.h>

static SYN_IMGUI ui;
static bool heater_enabled = false;
static int target_temp = 25;

void render_gui(void) {
    syn_imgui_begin_frame(&ui);

    syn_imgui_label(&ui, 10, 5, "Temperature Control");

    // Interactive Checkbox widget
    if (syn_imgui_checkbox(&ui, 10, 20, "Enable Heater", &heater_enabled)) {
        // Toggle callback logic
    }

    // Interactive Slider widget
    if (heater_enabled) {
        syn_imgui_slider(&ui, 10, 40, 100, 10, "Set Temp", &target_temp, 15, 40);
    }

    syn_imgui_end_frame(&ui);
}
```

# Input / Output Modules

SyntropicOS provides non-blocking, debounced drivers for physical user inputs (Buttons, Rotary Encoders) and output indicators (LEDs, Software PWM).

---

## 1. Button Driver (`syn_button.h`)

The `syn_button` driver provides debounced GPIO button management with built-in support for:
- Debouncing filter with configurable `debounce_ms` window.
- Active-High and Active-Low pin polarities.
- Multi-click tap detection (Single, Double, Triple, N-click).
- Long-press hold thresholds and auto-repeat streaming.
- Multi-button chorded combinations (`SYN_ButtonCombo`).

### Basic Initialization & Single Click

```c
#include <syntropic/input/syn_button.h>

static SYN_Button btn1;

void on_single_click(SYN_Button *btn, void *ctx) {
    // Fired after multi-click window (250ms) expires
}

void app_setup(void) {
    // Pin 2, Active-Low (pull-up), 50ms debounce
    syn_button_init(&btn1, 2, SYN_BUTTON_ACTIVE_LOW, 50);
    syn_button_on_single_click(&btn1, on_single_click, NULL);
}

void app_loop(void) {
    syn_button_update(&btn1);
}
```

### Multi-Click & Long-Press Configuration

```c
// Configure 200ms multi-click evaluation window
syn_button_set_click_window(&btn1, 200);

// Register tap gesture handlers
syn_button_on_single_click(&btn1, on_single, NULL);
syn_button_on_double_click(&btn1, on_double, NULL);
syn_button_on_triple_click(&btn1, on_triple, NULL);
syn_button_on_multi_click(&btn1, on_multi, NULL); // 4+ taps

// Register long press (fires ONCE at 1000ms hold threshold)
syn_button_on_long_press(&btn1, on_long_press, 1000, NULL);

// Register auto-repeat (fires every 100ms while held)
syn_button_on_repeat(&btn1, on_repeat, 100, NULL);
```

> [!NOTE]
> Entering a **Long Press** (`on_long_press`) or firing **Auto-Repeat** (`on_repeat`) automatically resets the click counter (`click_count = 0`), preventing single-click callbacks from triggering when the button is released.

### Multi-Button Combinations (`SYN_ButtonCombo`)

`SYN_ButtonCombo` enables chorded multi-button presses (e.g. pressing `BTN1` + `BTN2` simultaneously).

```c
static SYN_Button btn1;
static SYN_Button btn2;
static SYN_ButtonCombo combo;
static const SYN_Button *combo_list[2];

void on_combo_fired(SYN_Button *btn, void *ctx) {
    // Fired when BTN1 and BTN2 are both pressed
}

void app_setup(void) {
    syn_button_init(&btn1, 2, SYN_BUTTON_ACTIVE_LOW, 50);
    syn_button_init(&btn2, 3, SYN_BUTTON_ACTIVE_LOW, 50);

    combo_list[0] = &btn1;
    combo_list[1] = &btn2;
    syn_button_combo_init(&combo, combo_list, 2, on_combo_fired, NULL);
}

void app_loop(void) {
    syn_button_update(&btn1);
    syn_button_update(&btn2);
    syn_button_combo_update(&combo);
}
```

---

## 2. Rotary Encoder Driver (`syn_encoder.h`)

Quadrature rotary encoder decoding with velocity tracking and direction calculation.

| Function | Description |
|---|---|
| `syn_encoder_init()` | Initialize phase A & B GPIO pins |
| `syn_encoder_update()` | Sample phase pins and update step counter |
| `syn_encoder_get_value()` | Read relative step count |
| `syn_encoder_get_velocity()` | Read rotational velocity |

---

## 3. Non-Blocking LED Driver (`syn_led.h`)

Provides pattern blinking, flash sequences, and Morse-code style pattern strings.

```c
#include <syntropic/output/syn_led.h>

static SYN_LED status_led;

void setup(void) {
    syn_led_init(&status_led, LED_BUILTIN, SYN_LED_ACTIVE_HIGH);
    
    // Blink 500ms ON / 500ms OFF
    syn_led_blink(&status_led, 500, 500);
}

void loop(void) {
    syn_led_update(&status_led);
}
```

---

## 4. Software PWM Driver (`syn_soft_pwm.h`)

Enables software PWM output on any arbitrary GPIO pin without requiring hardware timers.

| Function | Description |
|---|---|
| `syn_soft_pwm_init()` | Initialize pin and frequency |
| `syn_soft_pwm_set_duty()` | Set duty cycle (0–255 or 0–100%) |
| `syn_soft_pwm_update()` | Service software PWM toggle loop |

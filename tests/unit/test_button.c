/**
 * @file test_button.c
 * @brief Unity tests for syn_button — full coverage & user behavior testing.
 */

#include "mocks/mock_port.h"
#include "syntropic/input/syn_button.h"
#include "syntropic/syntropic.h"
#include "unity/unity.h"

static int btn_press_count = 0;
static int btn_release_count = 0;
static int btn_long_count = 0;
static int btn_repeat_count = 0;

static void btn_on_press(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    btn_press_count++;
}
static void btn_on_release(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    btn_release_count++;
}
static void btn_on_long(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    btn_long_count++;
}
static void btn_on_repeat(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    btn_repeat_count++;
}

static int btn_single_click_count = 0;
static int btn_double_click_count = 0;
static int btn_triple_click_count = 0;
static int btn_multi_click_count = 0;
static int combo_fired_count = 0;

static void btn_on_single_click(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    btn_single_click_count++;
}
static void btn_on_double_click(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    btn_double_click_count++;
}
static void btn_on_triple_click(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    btn_triple_click_count++;
}
static void btn_on_multi_click(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    btn_multi_click_count++;
}
static void combo_on_fire(SYN_Button *b, void *ctx)
{
    (void)b;
    (void)ctx;
    combo_fired_count++;
}

static void reset_counts(void)
{
    btn_press_count = btn_release_count = btn_long_count = btn_repeat_count = 0;
    btn_single_click_count = btn_double_click_count = btn_triple_click_count =
        btn_multi_click_count = 0;
    combo_fired_count = 0;
}

/* ── Original test — ACTIVE_HIGH, preserved ──────────────────────────────── */

static void test_button(void)
{
    mock_tick_ms = 0;
    reset_counts();

    /* Pin 0 — we control it via gpio_states (from mock GPIO port) */
    mock_gpio_states[0] = 0; /* not pressed */

    SYN_Button btn;
    syn_button_init(&btn, 0, SYN_BUTTON_ACTIVE_HIGH, 50);
    syn_button_on_press(&btn, btn_on_press, NULL);
    syn_button_on_release(&btn, btn_on_release, NULL);
    syn_button_on_long_press(&btn, btn_on_long, 500, NULL);

    /* No press — should stay idle */
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(0, btn_press_count);
    TEST_ASSERT_FALSE(syn_button_is_pressed(&btn));

    /* Press the button */
    mock_gpio_states[0] = 1;
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(0, btn_press_count);

    /* Advance past debounce window */
    mock_tick_advance(60);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_press_count);
    TEST_ASSERT_TRUE(syn_button_is_pressed(&btn));

    /* Hold for long press */
    mock_tick_advance(500);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_long_count);

    /* Release */
    mock_gpio_states[0] = 0;
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_release_count);
    TEST_ASSERT_FALSE(syn_button_is_pressed(&btn));

    /* Bounce rejection: press then release before debounce */
    btn_press_count = 0;
    mock_gpio_states[0] = 1;
    mock_tick_advance(10);
    syn_button_update(&btn);
    mock_gpio_states[0] = 0; /* bounce off */
    mock_tick_advance(10);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(0, btn_press_count);

    /* Event polling */
    btn.events = 0;
    mock_gpio_states[0] = 1;
    syn_button_update(&btn);
    mock_tick_advance(60);
    syn_button_update(&btn);
    uint8_t evts = syn_button_poll_events(&btn);
    TEST_ASSERT_TRUE(evts & SYN_BUTTON_EVT_PRESS);
    TEST_ASSERT_EQUAL_INT(0, syn_button_poll_events(&btn));
}

/* ── Test: ACTIVE_LOW polarity ───────────────────────────────────────────── */

static void test_button_active_low(void)
{
    mock_tick_ms = 0;
    reset_counts();

    /* Pin 1 — ACTIVE_LOW: GPIO_LOW = pressed */
    mock_gpio_states[1] = SYN_GPIO_HIGH; /* not pressed */

    SYN_Button btn;
    syn_button_init(&btn, 1, SYN_BUTTON_ACTIVE_LOW, 50);
    syn_button_on_press(&btn, btn_on_press, NULL);
    syn_button_on_release(&btn, btn_on_release, NULL);

    /* Not pressed — no event */
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(0, btn_press_count);

    /* Press = drive LOW */
    mock_gpio_states[1] = SYN_GPIO_LOW;
    syn_button_update(&btn);

    mock_tick_advance(60);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_press_count);
    TEST_ASSERT_TRUE(syn_button_is_pressed(&btn));

    /* Release = drive HIGH */
    mock_gpio_states[1] = SYN_GPIO_HIGH;
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_release_count);
}

/* ── Test: repeat in PRESSED state ──────────────────────────────────────── */

static void test_button_repeat_in_pressed(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[2] = 0;
    SYN_Button btn;
    syn_button_init(&btn, 2, SYN_BUTTON_ACTIVE_HIGH, 50);
    syn_button_on_press(&btn, btn_on_press, NULL);
    syn_button_on_repeat(&btn, btn_on_repeat, 200, NULL); /* repeat every 200ms */

    /* Press → debounce */
    mock_gpio_states[2] = 1;
    syn_button_update(&btn);
    mock_tick_advance(60);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_press_count);

    /* Hold without long-press threshold — stay in PRESSED state */
    /* Advance past one repeat interval */
    mock_tick_advance(210);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_repeat_count); /* first repeat fired */

    mock_tick_advance(210);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(2, btn_repeat_count); /* second repeat */
}

/* ── Test: repeat in HELD state ─────────────────────────────────────────── */

static void test_button_repeat_in_held(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[3] = 0;
    SYN_Button btn;
    syn_button_init(&btn, 3, SYN_BUTTON_ACTIVE_HIGH, 50);
    syn_button_on_press(&btn, btn_on_press, NULL);
    syn_button_on_long_press(&btn, btn_on_long, 300, NULL);
    syn_button_on_repeat(&btn, btn_on_repeat, 150, NULL);

    /* Press → debounce → confirm */
    mock_gpio_states[3] = 1;
    syn_button_update(&btn);
    mock_tick_advance(60);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_press_count);

    /* Hold past long-press threshold → move to HELD state */
    mock_tick_advance(310);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_long_count);

    /* Now in HELD state — advance past repeat interval */
    mock_tick_advance(160);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_repeat_count);

    mock_tick_advance(160);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(2, btn_repeat_count);

    /* Release from HELD state */
    mock_gpio_states[3] = 0;
    syn_button_update(&btn);
    TEST_ASSERT_FALSE(syn_button_is_pressed(&btn));
}

/* ── Test: syn_button_service (batch update) ─────────────────────────────── */

static void test_button_service(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[4] = 0;
    mock_gpio_states[5] = 0;

    SYN_Button btns[2];
    syn_button_init(&btns[0], 4, SYN_BUTTON_ACTIVE_HIGH, 50);
    syn_button_init(&btns[1], 5, SYN_BUTTON_ACTIVE_HIGH, 50);
    syn_button_on_press(&btns[0], btn_on_press, NULL);
    syn_button_on_press(&btns[1], btn_on_press, NULL);

    /* Press both buttons */
    mock_gpio_states[4] = 1;
    mock_gpio_states[5] = 1;
    syn_button_service(btns, 2);

    mock_tick_advance(60);
    syn_button_service(btns, 2);

    /* Both should have fired press events */
    TEST_ASSERT_EQUAL_INT(2, btn_press_count);

    /* service with count=0 — should not crash */
    syn_button_service(btns, 0);
}

/* ── Test: Multi-click detection (single, double, triple, multi) ───────────── */

static void test_button_multi_clicks(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[6] = 0;
    SYN_Button btn;
    syn_button_init(&btn, 6, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_set_click_window(&btn, 200);

    syn_button_on_single_click(&btn, btn_on_single_click, NULL);
    syn_button_on_double_click(&btn, btn_on_double_click, NULL);
    syn_button_on_triple_click(&btn, btn_on_triple_click, NULL);
    syn_button_on_multi_click(&btn, btn_on_multi_click, NULL);

    /* 1. Single click test: press, debounce, release, wait window expiry */
    mock_gpio_states[6] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);
    mock_gpio_states[6] = 0;
    syn_button_update(&btn);

    /* Before window expiry — no single click yet */
    mock_tick_advance(100);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(0, btn_single_click_count);

    /* Advance past 200ms window */
    mock_tick_advance(150);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_single_click_count);

    /* 2. Double click test */
    reset_counts();
    /* Tap 1 */
    mock_gpio_states[6] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);
    mock_gpio_states[6] = 0;
    syn_button_update(&btn);

    /* Tap 2 within 200ms */
    mock_tick_advance(80);
    mock_gpio_states[6] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);
    mock_gpio_states[6] = 0;
    syn_button_update(&btn);

    /* Advance past window */
    mock_tick_advance(210);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(0, btn_single_click_count);
    TEST_ASSERT_EQUAL_INT(1, btn_double_click_count);

    /* 3. Triple click test */
    reset_counts();
    for (int i = 0; i < 3; i++) {
        mock_gpio_states[6] = 1;
        syn_button_update(&btn);
        mock_tick_advance(25);
        syn_button_update(&btn);
        mock_gpio_states[6] = 0;
        syn_button_update(&btn);
        mock_tick_advance(50);
    }
    mock_tick_advance(210);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_triple_click_count);

    /* 4. Multi-click (4 taps) test */
    reset_counts();
    for (int i = 0; i < 4; i++) {
        mock_gpio_states[6] = 1;
        syn_button_update(&btn);
        mock_tick_advance(25);
        syn_button_update(&btn);
        mock_gpio_states[6] = 0;
        syn_button_update(&btn);
        mock_tick_advance(50);
    }
    mock_tick_advance(210);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_multi_click_count);
}

/* ── Test: Combo button (simultaneous multi-button press) ─────────────── */

static void test_button_combo(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[7] = 0;
    mock_gpio_states[8] = 0;

    SYN_Button btn1, btn2;
    syn_button_init(&btn1, 7, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_init(&btn2, 8, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_set_click_window(&btn1, 200);
    syn_button_set_click_window(&btn2, 200);
    syn_button_on_single_click(&btn1, btn_on_single_click, NULL);
    syn_button_on_single_click(&btn2, btn_on_single_click, NULL);
    syn_button_on_long_press(&btn1, btn_on_long, 300, NULL);
    syn_button_on_long_press(&btn2, btn_on_long, 300, NULL);
    syn_button_on_repeat(&btn1, btn_on_repeat, 100, NULL);
    syn_button_on_repeat(&btn2, btn_on_repeat, 100, NULL);

    const SYN_Button *combo_btns[2] = {&btn1, &btn2};
    SYN_ButtonCombo combo;
    syn_button_combo_init(&combo, combo_btns, 2, combo_on_fire, NULL);

    TEST_ASSERT_FALSE(syn_button_combo_is_active(&combo));

    /* Press btn1 only */
    mock_gpio_states[7] = 1;
    syn_button_update(&btn1);
    syn_button_update(&btn2);
    mock_tick_advance(25);
    syn_button_update(&btn1);
    syn_button_update(&btn2);
    syn_button_combo_update(&combo);
    TEST_ASSERT_EQUAL_INT(0, combo_fired_count);
    TEST_ASSERT_FALSE(syn_button_combo_is_active(&combo));

    /* Now press btn2 as well */
    mock_gpio_states[8] = 1;
    syn_button_update(&btn1);
    syn_button_update(&btn2);
    mock_tick_advance(25);
    syn_button_update(&btn1);
    syn_button_update(&btn2);
    syn_button_combo_update(&combo);

    TEST_ASSERT_EQUAL_INT(1, combo_fired_count);
    TEST_ASSERT_TRUE(syn_button_combo_is_active(&combo));

    /* Advance time by 1000ms while holding both buttons in combo state */
    for (int i = 0; i < 100; i++) {
        mock_tick_advance(10);
        syn_button_update(&btn1);
        syn_button_update(&btn2);
        syn_button_combo_update(&combo);
    }

    /* Long press and repeat should BE SUPPRESSED for member buttons during combo */
    TEST_ASSERT_EQUAL_INT(0, btn_long_count);
    TEST_ASSERT_EQUAL_INT(0, btn_repeat_count);

    /* Release btn1 and btn2 */
    mock_gpio_states[7] = 0;
    mock_gpio_states[8] = 0;
    syn_button_update(&btn1);
    syn_button_update(&btn2);
    syn_button_combo_update(&combo);
    TEST_ASSERT_FALSE(syn_button_combo_is_active(&combo));

    /* Advance past click window (200ms) to ensure single clicks do NOT trigger */
    mock_tick_advance(210);
    syn_button_update(&btn1);
    syn_button_update(&btn2);
    TEST_ASSERT_EQUAL_INT(0, btn_single_click_count);
}

/* ── Test: 3-Button Combo & Staggered Release ───────────────────────────── */

static void test_button_combo_3_buttons(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[10] = 0;
    mock_gpio_states[11] = 0;
    mock_gpio_states[12] = 0;

    SYN_Button b1, b2, b3;
    syn_button_init(&b1, 10, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_init(&b2, 11, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_init(&b3, 12, SYN_BUTTON_ACTIVE_HIGH, 20);

    syn_button_on_single_click(&b1, btn_on_single_click, NULL);
    syn_button_on_single_click(&b2, btn_on_single_click, NULL);
    syn_button_on_single_click(&b3, btn_on_single_click, NULL);
    syn_button_on_long_press(&b1, btn_on_long, 200, NULL);
    syn_button_on_long_press(&b2, btn_on_long, 200, NULL);
    syn_button_on_long_press(&b3, btn_on_long, 200, NULL);

    const SYN_Button *trio[3] = {&b1, &b2, &b3};
    SYN_ButtonCombo combo3;
    syn_button_combo_init(&combo3, trio, 3, combo_on_fire, NULL);

    /* Press only b1 and b2 */
    mock_gpio_states[10] = 1;
    mock_gpio_states[11] = 1;
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_update(&b3);
    mock_tick_advance(25);
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_update(&b3);
    syn_button_combo_update(&combo3);
    TEST_ASSERT_FALSE(syn_button_combo_is_active(&combo3));
    TEST_ASSERT_EQUAL_INT(0, combo_fired_count);

    /* Press b3 to complete trio */
    mock_gpio_states[12] = 1;
    syn_button_update(&b3);
    mock_tick_advance(25);
    syn_button_update(&b3);
    syn_button_combo_update(&combo3);
    TEST_ASSERT_TRUE(syn_button_combo_is_active(&combo3));
    TEST_ASSERT_EQUAL_INT(1, combo_fired_count);

    /* Hold trio for 500ms */
    mock_tick_advance(500);
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_update(&b3);
    syn_button_combo_update(&combo3);
    TEST_ASSERT_EQUAL_INT(0, btn_long_count);

    /* Release b1 first while b2 and b3 remain held */
    mock_gpio_states[10] = 0;
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_update(&b3);
    syn_button_combo_update(&combo3);
    TEST_ASSERT_FALSE(syn_button_combo_is_active(&combo3));

    /* Release b2 and b3 */
    mock_gpio_states[11] = 0;
    mock_gpio_states[12] = 0;
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_update(&b3);
    syn_button_combo_update(&combo3);

    mock_tick_advance(250);
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_update(&b3);
    TEST_ASSERT_EQUAL_INT(0, btn_single_click_count);
}

/* ── Test: Helper Queries & NULL Parameters ─────────────────────────────── */

static void test_button_queries_and_null_checks(void)
{
    mock_tick_ms = 100;
    SYN_Button btn;
    syn_button_init(&btn, 13, SYN_BUTTON_ACTIVE_HIGH, 20);

    TEST_ASSERT_EQUAL_UINT32(0, syn_button_held_ms(&btn));
    TEST_ASSERT_EQUAL_UINT8(0, syn_button_clicks(&btn));
    TEST_ASSERT_FALSE(syn_button_combo_is_active(NULL));

    /* Combo update with NULL / empty list */
    SYN_ButtonCombo empty_combo;
    syn_button_combo_init(&empty_combo, NULL, 0, NULL, NULL);
    syn_button_combo_update(&empty_combo);
    TEST_ASSERT_FALSE(syn_button_combo_is_active(&empty_combo));

    const SYN_Button *null_btn_list[2] = {NULL, NULL};
    SYN_ButtonCombo null_combo;
    syn_button_combo_init(&null_combo, null_btn_list, 2, NULL, NULL);
    syn_button_combo_update(&null_combo);
    TEST_ASSERT_FALSE(syn_button_combo_is_active(&null_combo));
}

/* ── Test: Long Press / Repeat Release Prevents Spurious Single Click ───── */

static void test_button_long_press_release_no_single_click(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[14] = 0;
    SYN_Button btn;
    syn_button_init(&btn, 14, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_set_click_window(&btn, 250);
    syn_button_on_single_click(&btn, btn_on_single_click, NULL);
    syn_button_on_long_press(&btn, btn_on_long, 300, NULL);
    syn_button_on_repeat(&btn, btn_on_repeat, 100, NULL);

    /* Press button */
    mock_gpio_states[14] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);

    /* Hold button past long press threshold (300ms) */
    mock_tick_advance(310);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_long_count);

    /* Hold button for 300ms more with repeats */
    mock_tick_advance(300);
    syn_button_update(&btn);
    TEST_ASSERT_TRUE(btn_repeat_count > 0);

    /* Release button */
    mock_gpio_states[14] = 0;
    syn_button_update(&btn);

    /* Advance past click window (300ms) */
    mock_tick_advance(300);
    syn_button_update(&btn);

    /* SINGLE CLICK MUST NOT FIRE AFTER A LONG PRESS / REPEAT GESTURE */
    TEST_ASSERT_EQUAL_INT(0, btn_single_click_count);
}

/* ── Test: Polled Events & Immediate Click Window ───────────────────────── */

static void test_button_polled_events_and_immediate_click_window(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[15] = 0;
    SYN_Button btn;
    syn_button_init(&btn, 15, SYN_BUTTON_ACTIVE_HIGH, 20);

    /* Set click window to 0 -> immediate single click on release */
    syn_button_set_click_window(&btn, 0);
    syn_button_on_single_click(&btn, btn_on_single_click, NULL);

    /* Tap button */
    mock_gpio_states[15] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);
    mock_gpio_states[15] = 0;
    syn_button_update(&btn);

    /* With window = 0, single click fires immediately on next update in IDLE */
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_single_click_count);

    /* Verify polled event bitmask collects multiple events */
    btn.events = 0;
    mock_gpio_states[15] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);
    mock_gpio_states[15] = 0;
    syn_button_update(&btn);

    uint8_t evts = syn_button_poll_events(&btn);
    TEST_ASSERT_TRUE((evts & SYN_BUTTON_EVT_PRESS) != 0);
    TEST_ASSERT_TRUE((evts & SYN_BUTTON_EVT_RELEASE) != 0);
    TEST_ASSERT_EQUAL_UINT8(0, syn_button_poll_events(&btn));
}

/* ── Test: Slow Double Tap Registers Two Single Clicks ──────────────────── */

static void test_button_behavior_slow_double_tap(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[16] = 0;
    SYN_Button btn;
    syn_button_init(&btn, 16, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_set_click_window(&btn, 250);
    syn_button_on_single_click(&btn, btn_on_single_click, NULL);
    syn_button_on_double_click(&btn, btn_on_double_click, NULL);

    /* Tap 1 */
    mock_gpio_states[16] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);
    mock_gpio_states[16] = 0;
    syn_button_update(&btn);

    /* Wait 300ms (gap > double_click_ms 250) */
    mock_tick_advance(300);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_single_click_count);

    /* Tap 2 */
    mock_gpio_states[16] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);
    mock_gpio_states[16] = 0;
    syn_button_update(&btn);

    /* Wait 300ms */
    mock_tick_advance(300);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(2, btn_single_click_count);
    TEST_ASSERT_EQUAL_INT(0, btn_double_click_count);
}

/* ── Test: Double Tap and Hold Gesture ──────────────────────────────────── */

static void test_button_behavior_double_tap_and_hold(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[17] = 0;
    SYN_Button btn;
    syn_button_init(&btn, 17, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_set_click_window(&btn, 250);
    syn_button_on_single_click(&btn, btn_on_single_click, NULL);
    syn_button_on_double_click(&btn, btn_on_double_click, NULL);
    syn_button_on_long_press(&btn, btn_on_long, 300, NULL);

    /* Tap 1 */
    mock_gpio_states[17] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);
    mock_gpio_states[17] = 0;
    syn_button_update(&btn);

    /* Tap 2 (pressed down within 100ms, then held past 300ms) */
    mock_tick_advance(100);
    mock_gpio_states[17] = 1;
    syn_button_update(&btn);
    mock_tick_advance(25);
    syn_button_update(&btn);

    /* Hold past long press threshold (300ms) */
    mock_tick_advance(310);
    syn_button_update(&btn);
    TEST_ASSERT_EQUAL_INT(1, btn_long_count);

    /* Release */
    mock_gpio_states[17] = 0;
    syn_button_update(&btn);
    mock_tick_advance(300);
    syn_button_update(&btn);

    /* Double-click and single-click must NOT fire for tap-and-hold */
    TEST_ASSERT_EQUAL_INT(0, btn_single_click_count);
    TEST_ASSERT_EQUAL_INT(0, btn_double_click_count);
}

/* ── Test: Overlapping Independent Buttons ──────────────────────────────── */

static void test_button_behavior_overlapping_independent_buttons(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[18] = 0;
    mock_gpio_states[19] = 0;

    SYN_Button btnA, btnB;
    syn_button_init(&btnA, 18, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_init(&btnB, 19, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_set_click_window(&btnA, 250);
    syn_button_set_click_window(&btnB, 250);
    syn_button_on_single_click(&btnA, btn_on_single_click, NULL);
    syn_button_on_single_click(&btnB, btn_on_single_click, NULL);

    /* Press Btn A */
    mock_gpio_states[18] = 1;
    syn_button_update(&btnA);
    syn_button_update(&btnB);

    /* 10ms later, Press Btn B */
    mock_tick_advance(10);
    mock_gpio_states[19] = 1;
    syn_button_update(&btnA);
    syn_button_update(&btnB);

    /* Advance 25ms */
    mock_tick_advance(25);
    syn_button_update(&btnA);
    syn_button_update(&btnB);

    /* Release Btn A */
    mock_gpio_states[18] = 0;
    syn_button_update(&btnA);

    /* Release Btn B */
    mock_tick_advance(20);
    mock_gpio_states[19] = 0;
    syn_button_update(&btnB);

    /* Wait for click window expiry */
    mock_tick_advance(300);
    syn_button_update(&btnA);
    syn_button_update(&btnB);

    /* Both buttons must register 1 single click each (total 2) */
    TEST_ASSERT_EQUAL_INT(2, btn_single_click_count);
}

/* ── Test: Combo Reactivation Cycle ─────────────────────────────────────── */

static void test_button_behavior_combo_reactivation_cycle(void)
{
    mock_tick_ms = 0;
    reset_counts();

    mock_gpio_states[20] = 0;
    mock_gpio_states[21] = 0;

    SYN_Button b1, b2;
    syn_button_init(&b1, 20, SYN_BUTTON_ACTIVE_HIGH, 20);
    syn_button_init(&b2, 21, SYN_BUTTON_ACTIVE_HIGH, 20);

    const SYN_Button *pair[2] = {&b1, &b2};
    SYN_ButtonCombo combo;
    syn_button_combo_init(&combo, pair, 2, combo_on_fire, NULL);

    /* 1st Combo press */
    mock_gpio_states[20] = 1;
    mock_gpio_states[21] = 1;
    syn_button_update(&b1);
    syn_button_update(&b2);
    mock_tick_advance(25);
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_combo_update(&combo);
    TEST_ASSERT_EQUAL_INT(1, combo_fired_count);

    /* Release b1 while b2 is held */
    mock_gpio_states[20] = 0;
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_combo_update(&combo);
    TEST_ASSERT_FALSE(syn_button_combo_is_active(&combo));

    /* Re-press b1 while b2 is still held */
    mock_gpio_states[20] = 1;
    syn_button_update(&b1);
    syn_button_update(&b2);
    mock_tick_advance(25);
    syn_button_update(&b1);
    syn_button_update(&b2);
    syn_button_combo_update(&combo);

    /* Combo should reactivate and fire 2nd time! */
    TEST_ASSERT_EQUAL_INT(2, combo_fired_count);
    TEST_ASSERT_TRUE(syn_button_combo_is_active(&combo));
}

void run_button_tests(void)
{
    RUN_TEST(test_button);
    RUN_TEST(test_button_active_low);
    RUN_TEST(test_button_repeat_in_pressed);
    RUN_TEST(test_button_repeat_in_held);
    RUN_TEST(test_button_service);
    RUN_TEST(test_button_multi_clicks);
    RUN_TEST(test_button_combo);
    RUN_TEST(test_button_combo_3_buttons);
    RUN_TEST(test_button_queries_and_null_checks);
    RUN_TEST(test_button_long_press_release_no_single_click);
    RUN_TEST(test_button_polled_events_and_immediate_click_window);
    RUN_TEST(test_button_behavior_slow_double_tap);
    RUN_TEST(test_button_behavior_double_tap_and_hold);
    RUN_TEST(test_button_behavior_overlapping_independent_buttons);
    RUN_TEST(test_button_behavior_combo_reactivation_cycle);
}

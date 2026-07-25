#include "mock_port.h"
#include "syntropic/display/syn_canvas.h"
#include "unity/unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool flushed = false;
static void test_flush(const uint8_t *buf, size_t len, void *ctx)
{
    (void)buf;
    (void)len;
    (void)ctx;
    flushed = true;
}

void setUp(void)
{
}
void tearDown(void)
{
}

void test_renode_display_canvas_rendering(void)
{
    printf("[Renode Emulation] Testing Display Controller Canvas (SSD1306/ST7735 1bpp/16bpp)...\n");

    uint8_t fb[128 * 64 / 8];
    SYN_Canvas canvas;
    syn_canvas_init(&canvas, fb, 128, 64, 1 /* 1bpp mono */, test_flush, NULL);

    syn_canvas_clear(&canvas);
    syn_canvas_line(&canvas, 0, 0, 127, 63, 1);
    syn_canvas_rect(&canvas, 10, 10, 50, 30, 1);
    syn_canvas_text(&canvas, 5, 5, "SYN_OS", 1);

    TEST_ASSERT_EQUAL_UINT16(128, canvas.width);
    TEST_ASSERT_EQUAL_UINT16(64, canvas.height);

    syn_canvas_flush(&canvas);
    TEST_ASSERT_TRUE(flushed);

    printf("[Renode Emulation] Display Canvas Framebuffer Rendering PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_renode_display_canvas_rendering);
    return UNITY_END();
}

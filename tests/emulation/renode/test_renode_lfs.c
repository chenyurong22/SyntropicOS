#include "mock_port.h"
#include "syntropic/storage/syn_param.h"
#include "unity/unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    uint16_t brightness;
    int16_t offset;
    uint8_t mode;
} AppParams;

void setUp(void)
{
}
void tearDown(void)
{
}

void test_renode_param_wear_leveling(void)
{
    printf("[Renode Emulation] Testing SPI Flash Wear-Leveled Parameter Store (syn_param)...\n");

    SYN_ParamStore store;
    AppParams params;
    memset(&params, 0, sizeof(params));

    /* Initialize ParamStore at Flash Base 0x00 */
    SYN_Status status = syn_param_init(&store, 0x00, 2 /* sector count */, sizeof(AppParams));

    /* Save parameters */
    params.brightness = 85;
    params.offset = -12;
    params.mode = 0x02;

    status = syn_param_save(&store, &params);
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);

    /* Load parameters back */
    AppParams loaded_params;
    memset(&loaded_params, 0, sizeof(loaded_params));
    status = syn_param_load(&store, &loaded_params);
    TEST_ASSERT_EQUAL_INT(SYN_OK, status);

    TEST_ASSERT_EQUAL_UINT16(85, loaded_params.brightness);
    TEST_ASSERT_EQUAL_INT16(-12, loaded_params.offset);
    TEST_ASSERT_EQUAL_UINT8(0x02, loaded_params.mode);

    printf("[Renode Emulation] SPI Flash Wear-Leveled Parameter Save & Load PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_renode_param_wear_leveling);
    return UNITY_END();
}

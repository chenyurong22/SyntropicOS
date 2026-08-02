/**
 * @file test_comp.c
 * @brief Unit tests for High-Speed Analog Comparator (syn_comp) driver.
 */

#include "mocks/mock_port.h"
#include "syntropic/drivers/syn_comp.h"
#include "unity/unity.h"

extern bool mock_comp_output_state;

static void test_comp_init_and_control(void)
{
    SYN_COMP comp;

    /* Invalid handle check */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_comp_init(NULL, 0, SYN_COMP_INV_VREFINT));

    /* Valid initialization */
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_comp_init(&comp, 0, SYN_COMP_INV_VREFINT_1_2));
    TEST_ASSERT_EQUAL_UINT8(0, comp.comp_id);
    TEST_ASSERT_EQUAL_INT(SYN_COMP_INV_VREFINT_1_2, comp.inv_in);
    TEST_ASSERT_FALSE(comp.enabled);

    /* Output state reading */
    mock_comp_output_state = false;
    TEST_ASSERT_FALSE(syn_comp_read(NULL));
    TEST_ASSERT_FALSE(syn_comp_read(&comp));

    mock_comp_output_state = true;
    TEST_ASSERT_TRUE(syn_comp_read(&comp));

    /* Enable / disable */
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_comp_enable(NULL, true));
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_comp_enable(&comp, true));
    TEST_ASSERT_TRUE(comp.enabled);
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_comp_enable(&comp, false));
    TEST_ASSERT_FALSE(comp.enabled);
}

void run_comp_tests(void)
{
    RUN_TEST(test_comp_init_and_control);
}

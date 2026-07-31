/**
 * @file test_uds_util.c
 * @brief Unit tests for SAE J2012 / ISO 15031-6 UDS DTC conversion utilities.
 */

#include "syntropic/proto/syn_uds_util.h"
#include "unity/unity.h"

#include <string.h>

void test_uds_util_dtc_from_str(void)
{
    uint32_t dtc = 0;

    /* Powertrain P010500 */
    TEST_ASSERT_TRUE(syn_uds_dtc_from_str("P010500", &dtc));
    TEST_ASSERT_EQUAL_HEX32(0x010500U, dtc);

    /* Body B111717 */
    TEST_ASSERT_TRUE(syn_uds_dtc_from_str("B111717", &dtc));
    TEST_ASSERT_EQUAL_HEX32(0x911717U, dtc);

    /* Body B111716 */
    TEST_ASSERT_TRUE(syn_uds_dtc_from_str("b111716", &dtc));
    TEST_ASSERT_EQUAL_HEX32(0x911716U, dtc);

    /* Network U013100 */
    TEST_ASSERT_TRUE(syn_uds_dtc_from_str("U013100", &dtc));
    TEST_ASSERT_EQUAL_HEX32(0xC13100U, dtc);

    /* Chassis C101000 */
    TEST_ASSERT_TRUE(syn_uds_dtc_from_str("C101000", &dtc));
    TEST_ASSERT_EQUAL_HEX32(0x501000U, dtc);

    /* Short 5-character string "P0105" */
    TEST_ASSERT_TRUE(syn_uds_dtc_from_str("P0105", &dtc));
    TEST_ASSERT_EQUAL_HEX32(0x010500U, dtc);
}

void test_uds_util_dtc_to_str(void)
{
    char buf[16];

    /* 0x010500 -> "P010500" */
    TEST_ASSERT_TRUE(syn_uds_dtc_to_str(0x010500U, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("P010500", buf);

    /* 0x911717 -> "B111717" */
    TEST_ASSERT_TRUE(syn_uds_dtc_to_str(0x911717U, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("B111717", buf);

    /* 0xC13100 -> "U013100" */
    TEST_ASSERT_TRUE(syn_uds_dtc_to_str(0xC13100U, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("U013100", buf);
}

void test_uds_util_null_and_bounds(void)
{
    uint32_t dtc = 0;
    char buf[16];

    /* Null checks */
    TEST_ASSERT_FALSE(syn_uds_dtc_from_str(NULL, &dtc));
    TEST_ASSERT_FALSE(syn_uds_dtc_from_str("P010500", NULL));
    TEST_ASSERT_FALSE(syn_uds_dtc_to_str(0x010500U, NULL, sizeof(buf)));
    TEST_ASSERT_FALSE(syn_uds_dtc_to_str(0x010500U, buf, 7)); /* Buffer too small (< 8) */

    /* Invalid prefix string */
    TEST_ASSERT_FALSE(syn_uds_dtc_from_str("X010500", &dtc));

    /* Invalid length */
    TEST_ASSERT_FALSE(syn_uds_dtc_from_str("P01", &dtc));
    TEST_ASSERT_FALSE(syn_uds_dtc_from_str("P01050000", &dtc));

    /* Invalid hex character */
    TEST_ASSERT_FALSE(syn_uds_dtc_from_str("P010Z00", &dtc));
}

void run_uds_util_tests(void)
{
    RUN_TEST(test_uds_util_dtc_from_str);
    RUN_TEST(test_uds_util_dtc_to_str);
    RUN_TEST(test_uds_util_null_and_bounds);
}

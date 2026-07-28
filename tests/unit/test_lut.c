/**
 * @file test_lut.c
 * @brief Unity tests for syn_lut.
 */

#include "mocks/mock_port.h"
#include "syntropic/syntropic.h"
#include "syntropic/util/syn_lut.h"
#include "unity/unity.h"

static void test_lut(void)
{
    static const SYN_LUT_Entry table[] = {
        {0, 3300},
        {25, 2048},
        {50, 1200},
        {100, 400},
    };
    const size_t count = sizeof(table) / sizeof(table[0]);

    /* Forward: x→y */
    TEST_ASSERT_EQUAL_INT(3300, syn_lut_forward(table, count, 0));
    TEST_ASSERT_EQUAL_INT(400, syn_lut_forward(table, count, 100));
    TEST_ASSERT_EQUAL_INT(2048, syn_lut_forward(table, count, 25));

    /* Interpolation */
    int32_t y = syn_lut_forward(table, count, 75);
    /* Between (50,1200) and (100,400): y = 1200 + (400-1200)*(75-50)/(100-50) = 1200 - 400 = 800 */
    TEST_ASSERT_EQUAL_INT(800, y);

    /* Clamping */
    TEST_ASSERT_EQUAL_INT(3300, syn_lut_forward(table, count, -10));
    TEST_ASSERT_EQUAL_INT(400, syn_lut_forward(table, count, 200));

    /* Reverse: y→x (y is descending in this table) */
    int32_t x = syn_lut_reverse(table, count, 2048);
    TEST_ASSERT_EQUAL_INT(25, x);

    x = syn_lut_reverse(table, count, 800);
    TEST_ASSERT_EQUAL_INT(75, x);
}

static void test_lut_edge_cases(void)
{
    static const SYN_LUT_Entry flat_x[] = {{10, 100}, {10, 200}};
    static const SYN_LUT_Entry flat_y[] = {{10, 100}, {20, 100}};
    static const SYN_LUT_Entry asc_y[] = {{0, 0}, {50, 500}, {100, 1000}};

    /* count == 0 */
    TEST_ASSERT_EQUAL_INT(0, syn_lut_forward(NULL, 0, 50));
    TEST_ASSERT_EQUAL_INT(0, syn_lut_reverse(NULL, 0, 500));

    /* count == 1 */
    TEST_ASSERT_EQUAL_INT(10, syn_lut_reverse(flat_x, 1, 500));

    /* dx == 0 */
    TEST_ASSERT_EQUAL_INT(100, syn_lut_forward(flat_x, 2, 10));

    /* dy == 0 */
    TEST_ASSERT_EQUAL_INT(10, syn_lut_reverse(flat_y, 2, 100));

    /* Ascending y table reverse */
    TEST_ASSERT_EQUAL_INT(25, syn_lut_reverse(asc_y, 3, 250));

    /* Reverse out of bounds extrapolation */
    TEST_ASSERT_EQUAL_INT(200, syn_lut_reverse(asc_y, 3, 2000));
}

void run_lut_tests(void)
{
    RUN_TEST(test_lut);
    RUN_TEST(test_lut_edge_cases);
}

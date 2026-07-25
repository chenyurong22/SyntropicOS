/**
 * @file test_vector.c
 * @brief Unit tests for fixed-point vector operations and signal statistics.
 */

#include "syntropic/util/syn_vector.h"
#include "unity/unity.h"

#define Q16_TOL 50

static void ASSERT_Q16_NEAR(q16_t expected, q16_t actual, q16_t tol)
{
    q16_t diff = q16_abs(expected - actual);
    if (diff > tol) {
        TEST_FAIL_MESSAGE("Q16 values not equal within tolerance");
    }
}

void test_vector_basic_ops(void)
{
    q16_t a[4] = {Q16_FROM_INT(1), Q16_FROM_INT(2), Q16_FROM_INT(3), Q16_FROM_INT(4)};
    q16_t b[4] = {Q16_FROM_INT(10), Q16_FROM_INT(20), Q16_FROM_INT(30), Q16_FROM_INT(40)};
    q16_t out[4];

    syn_vec_add(a, b, out, 4);
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(11), out[0]);
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(44), out[3]);

    syn_vec_sub(b, a, out, 4);
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(9), out[0]);
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(36), out[3]);

    syn_vec_scale(a, Q16_FROM_INT(2), out, 4);
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(2), out[0]);
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(8), out[3]);
}

void test_vector_stats(void)
{
    q16_t v[5] = {Q16_FROM_INT(2), Q16_FROM_INT(4), Q16_FROM_INT(6), Q16_FROM_INT(8),
                  Q16_FROM_INT(10)};

    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(2), syn_vec_min(v, 5));
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(10), syn_vec_max(v, 5));
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(6), syn_vec_mean(v, 5));

    /* RMS of [2, 4, 6, 8, 10]: sqrt((4+16+36+64+100)/5) = sqrt(220/5) = sqrt(44) ≈ 6.633 */
    q16_t rms = syn_vec_rms(v, 5);
    ASSERT_Q16_NEAR(Q16_FROM_FRAC(6633, 1000), rms, Q16_TOL * 10);
}

void run_vector_tests(void)
{
    RUN_TEST(test_vector_basic_ops);
    RUN_TEST(test_vector_stats);
}

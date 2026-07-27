/**
 * @file test_dsp.c
 * @brief Unit tests for fixed-point Discrete Cosine Transform (DCT-II).
 */

#include "syntropic/util/syn_dsp.h"
#include "unity/unity.h"

#include <math.h>

void test_dsp_dct2_null_params(void)
{
    q7_t in[8] = {0};
    q7_t out[8] = {0};

    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dsp_dct2_q7(NULL, 8, out, 8));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dsp_dct2_q7(in, 8, NULL, 8));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dsp_dct2_q7(in, 0, out, 8));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dsp_dct2_q7(in, 8, out, 0));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_dsp_dct2_q7(in, 4, out, 8));
}

void test_dsp_dct2_dc_constant(void)
{
    /* Constant signal of 0.25 (32 in Q7) across 8 samples */
    q7_t in[8];
    for (int i = 0; i < 8; i++) {
        in[i] = Q7_FROM_FLOAT(0.25f);
    }

    q7_t coeffs[4];
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_dsp_dct2_q7(in, 8, coeffs, 4));

    /* For constant signal of 0.25, DC coefficient (k=0) compacts energy:
     * X_0 = sqrt(1/8) * 8 * 0.25 = 2.828 * 0.25 = 0.707 (in Q7 ~ 90)
     * AC coefficients (k > 0) should be near 0. */
    TEST_ASSERT_INT8_WITHIN(5, Q7_FROM_FLOAT(0.707f), coeffs[0]);
    TEST_ASSERT_INT8_WITHIN(4, 0, coeffs[1]);
    TEST_ASSERT_INT8_WITHIN(4, 0, coeffs[2]);
    TEST_ASSERT_INT8_WITHIN(4, 0, coeffs[3]);
}

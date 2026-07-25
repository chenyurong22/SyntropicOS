/**
 * @file test_quaternion.c
 * @brief Unit tests for fixed-point 3D Quaternion algebra.
 */

#include "syntropic/util/syn_quaternion.h"
#include "unity/unity.h"

#define Q16_TOL 50 /* ~0.0007 in Q16 */

static void ASSERT_Q16_NEAR(q16_t expected, q16_t actual, q16_t tol)
{
    q16_t diff = q16_abs(expected - actual);
    if (diff > tol) {
        TEST_FAIL_MESSAGE("Q16 values not equal within tolerance");
    }
}

void test_quaternion_identity_and_init(void)
{
    SYN_Quaternion q;
    syn_quat_identity(&q);
    TEST_ASSERT_EQUAL_INT32(Q16_ONE, q.w);
    TEST_ASSERT_EQUAL_INT32(0, q.x);
    TEST_ASSERT_EQUAL_INT32(0, q.y);
    TEST_ASSERT_EQUAL_INT32(0, q.z);
    TEST_ASSERT_EQUAL_INT32(Q16_ONE, syn_quat_norm(&q));

    syn_quat_init(&q, Q16_FROM_INT(1), Q16_FROM_INT(2), Q16_FROM_INT(3), Q16_FROM_INT(4));
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(1), q.w);
    TEST_ASSERT_EQUAL_INT32(Q16_FROM_INT(2), q.x);
}

void test_quaternion_mul_and_rotate(void)
{
    SYN_Quaternion q1, q2, q_prod;
    syn_quat_identity(&q1);
    syn_quat_identity(&q2);

    syn_quat_mul(&q1, &q2, &q_prod);
    TEST_ASSERT_EQUAL_INT32(Q16_ONE, q_prod.w);
    TEST_ASSERT_EQUAL_INT32(0, q_prod.x);

    /* Rotate vector (1, 0, 0) by 90 degrees around Z axis */
    SYN_Quaternion q_rot;
    syn_quat_from_euler(&q_rot, 0, 0, Q16_PI_2);

    q16_t v[3] = {Q16_ONE, 0, 0};
    q16_t v_out[3];
    syn_quat_rotate_vec3(&q_rot, v, v_out);

    ASSERT_Q16_NEAR(0, v_out[0], Q16_TOL);
    ASSERT_Q16_NEAR(Q16_ONE, v_out[1], Q16_TOL);
    ASSERT_Q16_NEAR(0, v_out[2], Q16_TOL);
}

void test_quaternion_euler_roundtrip(void)
{
    q16_t roll_in = Q16_FROM_FRAC(1, 4);  /* 0.25 rad */
    q16_t pitch_in = Q16_FROM_FRAC(1, 5); /* 0.20 rad */
    q16_t yaw_in = Q16_FROM_FRAC(1, 2);   /* 0.50 rad */

    SYN_Quaternion q;
    syn_quat_from_euler(&q, roll_in, pitch_in, yaw_in);

    q16_t roll_out, pitch_out, yaw_out;
    syn_quat_to_euler(&q, &roll_out, &pitch_out, &yaw_out);

    ASSERT_Q16_NEAR(roll_in, roll_out, Q16_TOL);
    ASSERT_Q16_NEAR(pitch_in, pitch_out, Q16_TOL);
    ASSERT_Q16_NEAR(yaw_in, yaw_out, Q16_TOL);
}

void test_quaternion_slerp(void)
{
    SYN_Quaternion q1, q2, q_mid;
    syn_quat_from_euler(&q1, 0, 0, 0);
    syn_quat_from_euler(&q2, 0, 0, Q16_PI_2);

    /* Halfway slerp (t = 0.5) should equal 45 degree rotation */
    syn_quat_slerp(&q1, &q2, Q16_HALF, &q_mid);

    q16_t roll, pitch, yaw;
    syn_quat_to_euler(&q_mid, &roll, &pitch, &yaw);

    ASSERT_Q16_NEAR(0, roll, Q16_TOL * 5);
    ASSERT_Q16_NEAR(0, pitch, Q16_TOL * 5);
    ASSERT_Q16_NEAR(Q16_PI / 4, yaw, Q16_TOL * 5);
}

void run_quaternion_tests(void)
{
    RUN_TEST(test_quaternion_identity_and_init);
    RUN_TEST(test_quaternion_mul_and_rotate);
    RUN_TEST(test_quaternion_euler_roundtrip);
    RUN_TEST(test_quaternion_slerp);
}

/**
 * @file test_flight_pid.c
 * @brief Unit tests for Zero-Heap 3-Axis Quadcopter Flight PID Stabilization & Motor Mixer.
 */

#include "syntropic/control/syn_flight_pid.h"
#include "unity/unity.h"

void test_flight_init(void)
{
    SYN_Flight_Controller fc;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_flight_init(&fc));
}

void test_flight_hover(void)
{
    SYN_Flight_Controller fc;
    syn_flight_init(&fc);

    SYN_Flight_IMU imu = {
        .gyro_roll = 0, .gyro_pitch = 0, .gyro_yaw = 0, .angle_roll = 0, .angle_pitch = 0};

    SYN_Flight_Commands cmd = {.throttle_us = 1500,
                               .roll_target = 0,
                               .pitch_target = 0,
                               .yaw_target = 0,
                               .angle_mode = false};

    SYN_Flight_MotorOutputs motors;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_flight_update(&fc, &imu, &cmd, 1, &motors));

    /* At hover (0 disturbance), all 4 motors must receive equal throttle (1500 us) */
    TEST_ASSERT_EQUAL_UINT16(1500, motors.m1);
    TEST_ASSERT_EQUAL_UINT16(1500, motors.m2);
    TEST_ASSERT_EQUAL_UINT16(1500, motors.m3);
    TEST_ASSERT_EQUAL_UINT16(1500, motors.m4);
}

void test_flight_roll_correction(void)
{
    SYN_Flight_Controller fc;
    syn_flight_init(&fc);

    /* Drone is rolling right (+50 deg/s disturbance) */
    SYN_Flight_IMU imu = {.gyro_roll = Q16_FROM_FLOAT(50.0f),
                          .gyro_pitch = 0,
                          .gyro_yaw = 0,
                          .angle_roll = 0,
                          .angle_pitch = 0};

    SYN_Flight_Commands cmd = {.throttle_us = 1500,
                               .roll_target = 0,
                               .pitch_target = 0,
                               .yaw_target = 0,
                               .angle_mode = false};

    SYN_Flight_MotorOutputs motors;
    syn_flight_update(&fc, &imu, &cmd, 1, &motors);

    /* Drone rolls right (+50 deg/s disturbance) -> Right motors (M1, M2) increase thrust to push
     * back left */
    TEST_ASSERT_TRUE(motors.m1 > motors.m3);
    TEST_ASSERT_TRUE(motors.m2 > motors.m4);
}

void test_flight_angle_mode(void)
{
    SYN_Flight_Controller fc;
    syn_flight_init(&fc);

    /* Drone is tilted right (+10 deg angle disturbance) */
    SYN_Flight_IMU imu = {.gyro_roll = 0,
                          .gyro_pitch = 0,
                          .gyro_yaw = 0,
                          .angle_roll = Q16_FROM_FLOAT(10.0f),
                          .angle_pitch = 0};

    SYN_Flight_Commands cmd = {
        .throttle_us = 1500,
        .roll_target = 0, /* Level target */
        .pitch_target = 0,
        .yaw_target = 0,
        .angle_mode = true /* Self-leveling angle mode */
    };

    SYN_Flight_MotorOutputs motors;
    syn_flight_update(&fc, &imu, &cmd, 10, &motors);

    /* Self-leveling should drive correction */
    TEST_ASSERT_TRUE(motors.m1 != motors.m3);
}

void test_flight_clamp_motor_outputs_bounds(void)
{
    SYN_Flight_Controller fc;
    syn_flight_init(&fc);

    SYN_Flight_IMU imu = {0};

    /* Extreme low throttle to hit clamp_us < 1000 */
    SYN_Flight_Commands cmd_low = {
        .throttle_us = 500, .roll_target = 0, .pitch_target = 0, .yaw_target = 0};
    SYN_Flight_MotorOutputs motors_low;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_flight_update(&fc, &imu, &cmd_low, 1, &motors_low));
    TEST_ASSERT_EQUAL_UINT16(1000, motors_low.m1);
    TEST_ASSERT_EQUAL_UINT16(1000, motors_low.m2);

    /* Extreme high throttle to hit clamp_us > 2000 */
    SYN_Flight_Commands cmd_high = {
        .throttle_us = 2500, .roll_target = 0, .pitch_target = 0, .yaw_target = 0};
    SYN_Flight_MotorOutputs motors_high;
    TEST_ASSERT_EQUAL_INT(SYN_OK, syn_flight_update(&fc, &imu, &cmd_high, 1, &motors_high));
    TEST_ASSERT_EQUAL_UINT16(2000, motors_high.m1);
    TEST_ASSERT_EQUAL_UINT16(2000, motors_high.m2);
}

void test_flight_null_and_bounds(void)
{
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_flight_init(NULL));
    TEST_ASSERT_EQUAL_INT(SYN_INVALID_PARAM, syn_flight_update(NULL, NULL, NULL, 1, NULL));
}

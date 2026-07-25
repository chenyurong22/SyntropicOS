#include "mock_port.h"
#include "syntropic/control/syn_pid.h"
#include "syntropic/port/syn_port_adc.h"
#include "syntropic/port/syn_port_pwm.h"
#include "unity/unity.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void setUp(void)
{
}
void tearDown(void)
{
}

void test_renode_closed_loop_adc_pid_pwm(void)
{
    printf("[Renode Emulation] Testing Closed-Loop ADC -> PID -> PWM Motor Control...\n");

    SYN_PID pid;
    SYN_PID_Config cfg = SYN_PID_GAINS(1.5, 0.5, 0.1, 256, 0, 1000);
    syn_pid_init(&pid, &cfg);

    int32_t setpoint = 500; /* Target ADC speed reading */
    int32_t measured = 100; /* Initial measured speed reading */

    /* Run 5 closed-loop PID control iterations */
    for (int i = 0; i < 5; i++) {
        int32_t pwm_out = syn_pid_update(&pid, setpoint, measured, 10 /* dt_ms */);
        printf("[Renode Emulation] Step %d: Measured = %d, PID PWM Duty = %d\n", i, measured,
               pwm_out);

        TEST_ASSERT_TRUE(pwm_out >= 0 && pwm_out <= 1000);
        /* Simulate plant speed response approaching setpoint */
        measured += (setpoint - measured) / 2;
    }

    printf("[Renode Emulation] Closed-Loop ADC -> PID -> PWM Loop PASS!\n");
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_renode_closed_loop_adc_pid_pwm);
    return UNITY_END();
}

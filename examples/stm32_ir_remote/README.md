# SyntropicOS STM32 IR Remote Control Example

Demonstrates zero-allocation, non-blocking Infrared (IR) remote control decoding and encoding (`syn_ir`) integrated with STM32 HAL.

## Features
- **IR Receiving**: Timer Input Capture (`HAL_TIM_IC_CaptureCallback`) measures pulse mark/space durations in microseconds and feeds `syn_ir_decode_pulse`.
- **IR Transmitting**: Timer PWM (`HAL_TIM_PWM_Start`) generates 38 kHz / 40 kHz carrier pulses from `syn_ir_encode_frame`.
- **Protocols Supported**: NEC, Sony SIRCS, RC5, RC6, Samsung, Kaseikyo, Denon, Apple IR.

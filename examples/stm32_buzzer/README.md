# SyntropicOS Piezo Buzzer Audio Tone & Melody STM32 HAL Example

Demonstrates non-blocking piezo buzzer audio tone generation (`SYN_Buzzer`), single beep alerts (`syn_buzzer_beep`), multi-note musical melody & alarm pattern playback (`syn_buzzer_play_pattern`), and tone progress stepping (`syn_buzzer_step`) using STM32 HAL GPIO or Timer PWM hardware drivers (`HAL_TIM_PWM_...`).

## Architecture & Features

- **Non-Blocking Tone Engine**: Plays audio tones and alert assume sequences without halting main loop execution using `HAL_Delay`.
- **Single Beep & Chime Alerts**: Triggers timed audio beeps with specified frequency ($200\text{Hz}..5\text{kHz}$) and duration (`syn_buzzer_beep`).
- **Melody & Alarm Sequences**: Sequentially steps through array pairs of note frequencies ($\text{Hz}$) and durations ($\text{ms}$) via `syn_buzzer_play_pattern`.
- **Playback Queries & Mute**: Provides real-time playback state verification (`syn_buzzer_is_playing`) and instant playback muting (`syn_buzzer_stop`).

## Hardware Wiring

```
+--------------------+                    +---------------------+
|  STM32 Micro       |                    |  Piezo Buzzer Module|
|  (e.g., STM32F4)   |                    |  (Active / Passive) |
|                    |                    |                     |
|  TIM3_CH1    (PA6) ---------------------> I/O (Pin 1)         |
|                GND ---------------------> GND (Pin 2)         |
+--------------------+                    +---------------------+
```

## Tone Frequencies & Audio Patterns

- **Key Click Chime**: 2000Hz tone for 30ms.
- **Success Chime**: C5 (523Hz, 100ms) $\rightarrow$ E5 (659Hz, 100ms) $\rightarrow$ G5 (784Hz, 200ms).
- **Alarm Tone Pattern**: Alternating 1000Hz / 2000Hz dual-tone siren (200ms per tone).
- **Step Interval**: 10ms state machine update rate (`syn_buzzer_step`).

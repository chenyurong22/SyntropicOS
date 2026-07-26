# DSP & Digital Signal Processing Modules

SyntropicOS provides fixed-point (integer-only) Digital Signal Processing (DSP) components designed for microcontrollers without Hardware Floating-Point Units (FPUs).

---

## Technical Specifications

| Feature | Specification |
|---|---|
| **Math Representation** | Q16.16 Fixed-Point (`q16_t`). 16 bits integer, 16 bits fractional. |
| **Accumulator Resolution** | 64-bit integer (`int64_t`) to prevent overflow during sum/square accumulation. |
| **Memory Allocation** | **100% Static / Zero Heap**. Buffers are caller-owned. |

---

## 1. Digital Filters (`dsp/syn_filter.h` & `dsp/syn_biquad.h`)

SyntropicOS includes Moving Average, Exponential Moving Average (EMA), Median spike rejection, Direct-Form FIR, and Butterworth Biquad IIR filters.

### Signal Processing Flow

```mermaid
flowchart LR
    RawADC["Raw ADC Sample"] --> MedianFilter["Median Filter (Spike Removal)"]
    MedianFilter --> EMAFilter["EMA / Biquad Filter (Noise Reduction)"]
    EMAFilter --> SignalStats["Signal Statistics (Min, Max, Mean, RMS)"]
```

### Complete Code Example (Biquad Butterworth Lowpass Filter)

```c
#include <syntropic/dsp/syn_biquad.h>
#include <syntropic/dsp/syn_filter.h>

static SYN_FilterBiquad lpf;
static SYN_FilterEMA    ema;

void dsp_init(void) {
    // 1. Initialize EMA Filter (alpha = 64/256 = 0.25)
    syn_filter_ema_init(&ema, 64);

    // 2. Initialize 2nd-order Butterworth Lowpass Filter: 100 Hz cutoff, 1000 Hz sample rate
    syn_filter_biquad_lowpass(&lpf, Q16_FROM_INT(100), Q16_FROM_INT(1000));
}

int16_t process_adc_sample(int16_t raw_sample) {
    // Convert sample to Q16.16 fixed-point format
    q16_t in_q16 = Q16_FROM_INT(raw_sample);

    // Filter through lowpass Biquad
    q16_t filtered_q16 = syn_filter_biquad_update(&lpf, in_q16);

    // Return integer result
    return (int16_t)Q16_TO_INT(filtered_q16);
}
```

---

## 2. Signal Statistics (`dsp/syn_signal.h`)

Provides real-time sliding window statistics (min, max, mean, variance, standard deviation, RMS) over a caller-owned circular buffer.

```c
#include <syntropic/dsp/syn_signal.h>

static int32_t stats_buffer[64];
static SYN_Signal sig;

void stats_init(void) {
    syn_signal_init(&sig, stats_buffer, 64);
}

void on_adc_sample(int32_t val) {
    syn_signal_push(&sig, val);

    int32_t min_val = syn_signal_min(&sig);
    int32_t max_val = syn_signal_max(&sig);
    int32_t mean_val = syn_signal_mean(&sig);
    int32_t rms_val  = syn_signal_rms_q16(&sig);
}
```

---

## 3. Fast Fourier Transform & Peak Detection (`dsp/syn_fft.h`)

Provides fixed-point Radix-2 FFT spectral analysis, Hanning/Hamming windowing, peak frequency identification, and Total Harmonic Distortion (THD) calculation.

```c
#include <syntropic/dsp/syn_fft.h>

#define FFT_SIZE 64

static q16_t real_buf[FFT_SIZE];
static q16_t imag_buf[FFT_SIZE];
static q16_t mag_buf[FFT_SIZE / 2];

void analyze_spectrum(void) {
    // Apply Hanning window
    syn_fft_apply_window(real_buf, FFT_SIZE, SYN_FFT_WINDOW_HANNING);

    // Perform Radix-2 FFT
    syn_fft_perform(real_buf, imag_buf, FFT_SIZE);

    // Compute magnitude spectrum
    syn_fft_magnitude_spectrum(real_buf, imag_buf, mag_buf, FFT_SIZE / 2);

    // Find dominant frequency peak index
    uint16_t peak_bin = syn_fft_find_peak(mag_buf, FFT_SIZE / 2);
}
```

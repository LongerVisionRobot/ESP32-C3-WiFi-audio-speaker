#pragma once

#include <math.h>
#include <stdint.h>

#define LUT_SIZE 512
#define AMPLITUDE 0.8f

static int16_t sine_lut[LUT_SIZE];

// Initialize the lookup table (sine wave)
static inline void generate_lut(void)
{
    for (int i = 0; i < LUT_SIZE; i++)
    {
        float theta = 2.0f * M_PI * i / LUT_SIZE;
        sine_lut[i] = (int16_t)(32767.0f * AMPLITUDE * sinf(theta));
    }
}

// t_float is an increasing floating-point value [0, 1, 2, ...],
// representing the position within the waveform cycle.
// freq is the frequency (Hz), and SAMPLE_RATE is the sampling rate (Hz)
static inline int16_t get_lut_sample_from_phase(float phase)
{
    int index = (int)(phase * LUT_SIZE);
    return sine_lut[index % LUT_SIZE];
}

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <Arduino.h>  // For Teensy/Arduino functions
#include "imxrt.h"  // Include Teensy 4.1 hardware definitions

#define TABLE_SIZE 256
#define SAMPLE_RATE 48000
#define PWM_PIN 9     // Teensy 4.1 PWM-capable pin (adjust as needed)
#define PWM_FREQ SAMPLE_RATE
#define PWM_RESOLUTION 8  // 8-bit resolution (0-255)

void loop() {
    // Main loop stays free for other tasks
}


// Precompute sine lookup table
float SINELUT[TABLE_SIZE];
void init_sineLUT() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        SINELUT[i] = sinf((2.0f * 3.14159265f * i) / TABLE_SIZE);
    }
}

// Waveform types
typedef enum {
    WAVE_SIN,
    WAVE_SAW,
    WAVE_SQU
} WaveType;

// Oscillator structure
typedef struct {
    float frequency;      // Frequency in Hz
    float phase;          // Current phase [0.0, 1.0)
    WaveType wave_type;
} oscillator;

// LFO structure
typedef struct {
    float depth;         // Modulation depth (0.0 - 1.0)
    float frequency;     // LFO frequency in Hz
    float phase;         // Current phase [0.0, 1.0)
    WaveType wave_type;
} lfo_filter;

// Synth parameters
typedef struct {
    oscillator osc;
    lfo_filter lfo;
} synth_params;

// Generate a single sample
float generate_sample(synth_params* params) {
    oscillator* osc = &params->osc;
    lfo_filter* lfo = &params->lfo;

    // Calculate LFO value
    float lfoValue = 0.0f;
    switch (lfo->wave_type) {
        case WAVE_SIN: {
            int lfoIndex = (int)(lfo->phase * TABLE_SIZE) % TABLE_SIZE;
            lfoValue = SINELUT[lfoIndex];
            break;
        }
        case WAVE_SAW:
            lfoValue = 2.0f * lfo->phase - 1.0f;
            break;
        case WAVE_SQU:
            lfoValue = (lfo->phase < 0.5f) ? -1.0f : 1.0f;
            break;
    }

    // Apply LFO to frequency modulation
    float frequencyMod = 1.0f + (lfoValue * lfo->depth);
    float phaseIncrement = (osc->frequency / SAMPLE_RATE) * frequencyMod;

    // Generate oscillator output
    float sample = 0.0f;
    switch (osc->wave_type) {
        case WAVE_SIN: {
            int index = (int)(osc->phase * TABLE_SIZE) % TABLE_SIZE;
            float fundamental = SINELUT[index];
            float secondHarmonic = SINELUT[(2 * index) % TABLE_SIZE];
            float thirdHarmonic = SINELUT[(3 * index) % TABLE_SIZE];
            sample = (fundamental + secondHarmonic + thirdHarmonic) / 3.0f;
            break;
        }
        case WAVE_SAW:
            sample = 2.0f * osc->phase - 1.0f;
            break;
        case WAVE_SQU:
            sample = (osc->phase < 0.5f) ? -1.0f : 1.0f;
            break;
    }

    // Update phases
    osc->phase += phaseIncrement;
    if (osc->phase >= 1.0f) osc->phase -= 1.0f;
    lfo->phase += lfo->frequency / SAMPLE_RATE;
    if (lfo->phase >= 1.0f) lfo->phase -= 1.0f;

    // Scale sample to PWM range (0 to 255)
    return (sample + 1.0f) * 127.5f;  // Convert [-1, 1] to [0, 255]
}

// Timer interrupt callback
volatile float nextSample = 0.0f;

void setup() {
    // Enable clock for the PIT
    CCM_CCGR1 |= CCM_CCGR1_PIT(CCM_CCGR_ON);

    // Load value for a 1 ms interval (adjust as needed)
    PIT_CHANNEL[0].LDVAL = (F_CPU / 1000) - 1;

    // Enable timer and interrupt
    PIT_MCR = 0;
    PIT_TCTRL0 = PIT_TCTRL_TIE | PIT_TCTRL_TEN;
    NVIC_ENABLE_IRQ(IRQ_PIT);
}

void pit_isr() {
    // Clear interrupt flag
    PIT_TFLG0 = PIT_TFLG_TIF;

    // Your buffer update logic here
}

void loop() {
    // Synth parameters
    static synth_params params = {
        .osc = {240.0f, 0.0f, WAVE_SIN},
        .lfo = {0.2f, 10.0f, 0.0f, WAVE_SAW}
    };

    // Generate next sample
    noInterrupts();  // Protect shared variable
    nextSample = generate_sample(&params);
    interrupts();

    delay(10);  // Adjust as needed for control updates
}

int main() {
    setup();
    init_sineLUT();
    while (1) {
        loop();
    }
    return 0;
}

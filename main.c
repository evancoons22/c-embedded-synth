#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#ifdef EMBEDDED
#include "embedded_delay.h"
#else
#include <unistd.h>
#include <pthread.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define TABLE_SIZE 256
#define SAMPLE_RATE 48000.0f

// Precompute a sine lookup table for one cycle.
float SINELUT[TABLE_SIZE];
void init_sineLUT() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        SINELUT[i] = sinf((2.0f * 3.14159265f * i) / TABLE_SIZE);
    }
}

// Define waveform types.
typedef enum {
    WAVE_SIN,
    WAVE_SAW,
    WAVE_SQU
} WaveType;

// Structure to hold oscillator state.
typedef struct {
    float frequency;      // Frequency in Hz.
    float phase;          // Current phase [0.0, 1.0).
    float phase_increment; // Not used directly; computed each callback.
    WaveType wave_type;
} oscillator;

typedef struct { 
    float depth;         // Modulation depth (0.0 - 1.0).
    float frequency;     // LFO frequency in Hz.
    float phase;         // Current phase of the LFO [0.0, 1.0).
    float phase_increment; // Not used directly; computed each callback.
    WaveType wave_type;   // LFO waveform type.
    float target;        // Parameter being modulated (e.g. base frequency).
} lfo_filter;

typedef struct { 
    oscillator osc;
    lfo_filter lfo;
} synth_params;

#ifndef EMBEDDED
// For Linux: set terminal to non-canonical mode for immediate keypress processing.
struct termios orig_termios;
void reset_terminal_mode(void) {
    tcsetattr(0, TCSANOW, &orig_termios);
}
void set_conio_terminal_mode(void) {
    struct termios new_termios;
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));
    new_termios.c_lflag &= ~(ICANON | ECHO); // Disable buffering and echo.
    tcsetattr(0, TCSANOW, &new_termios);
    atexit(reset_terminal_mode);
}
int kbhit(void) {
    int bytesWaiting;
    ioctl(0, FIONREAD, &bytesWaiting);
    return bytesWaiting;
}
int getch(void) {
    int r;
    unsigned char c;
    if ((r = (int)read(0, &c, sizeof(c))) < 0) {
        return r;
    } else {
        return c;
    }
}
#endif

#ifndef EMBEDDED
// Input thread: waits for 'j' or 'k' key to adjust the LFO frequency.
void* input_thread(void* arg) {
    synth_params* params = (synth_params*)arg;
    set_conio_terminal_mode();
    printf("Press 'j' to increase LFO frequency by 0.1 Hz, 'k' to decrease by 0.1 Hz\n");
    while (1) {
        if (kbhit()) {
            int ch = getch();
            if (ch == 'j') {
                params->lfo.frequency += 0.1f;
            } else if (ch == 'k') {
                params->lfo.frequency -= 0.1f;
                if (params->lfo.frequency < 0.0f) params->lfo.frequency = 0.0f;
            } else if (ch == 'g') { 
                params->osc.frequency += 1.0;
                if (params->osc.frequency > 600.0) params->osc.frequency = 600.0f;
            } else if (ch == 'h') {
                params->osc.frequency -= 1.0;
                if (params->osc.frequency < 100.0) params->osc.frequency = 100.0f;
            } else if (ch == 'd') { 
                params->lfo.depth += 0.1f;
                if (params->lfo.depth > 2.0f) params->lfo.depth = 2.0f;
            } else if (ch == 'f') {
                params->lfo.depth -= 0.1f;
                if (params->lfo.depth < 0.0f) params->lfo.depth = 0.0f;
            } else if (ch == 'w') {  
                params->osc.wave_type = (params->osc.wave_type + 1) % 3;
            } else if (ch == 'e') { 
                params->lfo.wave_type = (params->lfo.wave_type + 1) % 3;
            }
        }

        // Clear screen
        printf("\033[2J\033[H");

        // Print parameters with controls
        printf("C-EMBEDDED-SYNTH PARAMETERS\n");
        printf("---------------------------\n");
        printf("Oscillator Frequency: %.2f Hz    (g: increase, h: decrease)\n", params->osc.frequency);
        printf("LFO Frequency:        %.2f Hz    (j: increase, k: decrease)\n", params->lfo.frequency);
        printf("LFO Depth:            %.2f       (d: increase, f: decrease)\n", params->lfo.depth);
        printf("--------------------------------------------------------------------\n");
        printf("Oscillator Waveform:  %s         (w: cycle through waveforms)\n", 
                params->osc.wave_type == WAVE_SIN ? "Sine" : 
                params->osc.wave_type == WAVE_SAW ? "Sawtooth" : "Square");
        printf("LFO Waveform:         %s         (e: cycle through waveforms)\n",
                params->lfo.wave_type == WAVE_SIN ? "Sine" : 
                params->lfo.wave_type == WAVE_SAW ? "Sawtooth" : "Square");

        usleep(10000); // Sleep 10ms to reduce CPU load.
    }
    return NULL;
}
#endif

// Callback function that generates audio data.
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    synth_params* params = (synth_params*)pDevice->pUserData;
    oscillator* osc = &params->osc;
    lfo_filter* lfo = &params->lfo;
    float* out = (float*)pOutput;
    
    for (ma_uint32 i = 0; i < frameCount; i++) {
        float sample = 0.0f;
        float lfoValue = 0.0f;
        
        // Calculate LFO value based on current phase and waveform.
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
            default: {
                int lfoIndex = (int)(lfo->phase * TABLE_SIZE) % TABLE_SIZE;
                lfoValue = SINELUT[lfoIndex];
                break;
            }
        }
        
        // Apply LFO to frequency modulation.
        // Map lfoValue from [-1,1] to [1-depth, 1+depth].
        float frequencyMod = 1.0f + (lfoValue * lfo->depth);
        // float currentPhaseIncrement = (lfo->target / SAMPLE_RATE) * frequencyMod;
        float currentPhaseIncrement = (osc->frequency / SAMPLE_RATE) * frequencyMod;
        
        // Generate oscillator output using the modulated phase increment.
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
            default: {
                int index = (int)(osc->phase * TABLE_SIZE) % TABLE_SIZE;
                sample = SINELUT[index];
                break;
            }
        }
        
        // Write stereo sample.
        *out++ = sample;
        *out++ = sample;
        
        // Update oscillator phase.
        osc->phase += currentPhaseIncrement;
        if (osc->phase >= 1.0f)
            osc->phase -= 1.0f;
        
        // Update LFO phase.
        lfo->phase += lfo->frequency / SAMPLE_RATE;
        if (lfo->phase >= 1.0f)
            lfo->phase -= 1.0f;
    }
}

#ifdef EMBEDDED
int main(void) {
#else
int main() {
#endif
    init_sineLUT();
    
    // Initialize synth parameters.
    synth_params params;
    params.osc.frequency = 240.0f;
    params.osc.phase = 0.0f;
    params.osc.phase_increment = params.osc.frequency / SAMPLE_RATE;
    params.osc.wave_type = WAVE_SIN;
    
    params.lfo.depth = 0.2f;
    params.lfo.frequency = 10.0f;
    params.lfo.phase = 0.0f;
    params.lfo.phase_increment = params.lfo.frequency / SAMPLE_RATE;
    params.lfo.wave_type = WAVE_SAW;  // You can change this to WAVE_SIN or WAVE_SQU.
    params.lfo.target = params.osc.frequency; // defining the target as the oscialltor frequency. that is what we will modify
    
    // Configure miniaudio.
    ma_device device;
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;
    config.playback.channels = 2;
    config.sampleRate        = 48000;
    config.dataCallback      = data_callback;
    config.pUserData         = &params;
    
    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to initialize audio device.\n");
        return -1;
    }
    if (ma_device_start(&device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start audio device.\n");
        ma_device_uninit(&device);
        return -1;
    }
    
#ifndef EMBEDDED
    // On Linux, start the input thread to adjust LFO frequency.
    pthread_t thread;
    if (pthread_create(&thread, NULL, input_thread, &params) != 0) {
        fprintf(stderr, "Error creating input thread.\n");
        return -1;
    }
    // printf("Playing rich tone. Press 'j' or 'k' to adjust LFO frequency.\n");
    // Run for 30 seconds, for example.
    sleep(30);
    pthread_cancel(thread);
#else
    // On embedded, poll physical buttons here instead.
    while (1) {
        embedded_delay_ms(10);
        // Update button status and adjust params.lfo.frequency accordingly.
    }
#endif
    
    ma_device_uninit(&device);
    return 0;
}


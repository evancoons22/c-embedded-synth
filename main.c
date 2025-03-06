#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#ifdef EMBEDDED
    #define MA_NO_RUNTIME_LINKING
    #define MA_NO_THREADING
    #define MA_NO_DEVICE_IO
    #ifdef ARDUINO
        #include <Arduino.h>
        static inline void embedded_delay_ms(uint32_t ms) { delay(ms); }
    #else
        #include <unistd.h>
        static inline void embedded_delay_ms(uint32_t ms) { usleep(ms * 1000); }
    #endif
#else
#include <pthread.h>
#include <termios.h>
#include <sys/ioctl.h>
#endif

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define TABLE_SIZE 1024
#define SAMPLE_RATE 48000.0f
#define MAX_VOICES 5

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
    float base_freq;
    float freqs[MAX_VOICES];
    float phase;          // Current phase [0.0, 1.0).
    float phases[MAX_VOICES];
    WaveType wave_type;
    int num_voices;
} oscillator;

typedef struct { 
    float depth;        
    float base_freq;     
    float phase;         
    WaveType wave_type;   
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

void* input_thread(void* arg) {
    synth_params* params = (synth_params*)arg;
    set_conio_terminal_mode();
    printf("Press 'j' to increase LFO base_freq by 0.1 Hz, 'k' to decrease by 0.1 Hz\n");
    while (1) {
        if (kbhit()) {
            int ch = getch();
            if (ch == 'j') {
                params->lfo.base_freq += 0.1f;
            } else if (ch == 'k') {
                params->lfo.base_freq -= 0.1f;
                if (params->lfo.base_freq < 0.0f) params->lfo.base_freq = 0.0f;
            } else if (ch == 'g') { 
                params->osc.base_freq += 10.0;
                for (int k = 0; k < MAX_VOICES; k ++) { 
                    if (params->osc.freqs[k] > 600.0) params->osc.freqs[k] = 600.0f;
                    params->osc.freqs[k] += 10.0f;
                }
                if (params->osc.base_freq > 600.0) params->osc.base_freq = 600.0f;
            } else if (ch == 'h') {
                params->osc.base_freq -= 10.0;
                for (int k = 0; k < MAX_VOICES; k ++) { 
                    if (params->osc.freqs[k] < 50.0) params->osc.freqs[k] = 50.0f;
                    params->osc.freqs[k] -= 10.0f;
                }
                if (params->osc.base_freq < 50.0) params->osc.base_freq = 50.0f;
            } else if (ch == 'd') { 
                params->lfo.depth += 0.05f;
                if (params->lfo.depth > 2.0f) params->lfo.depth = 2.0f;
            } else if (ch == 'f') {
                params->lfo.depth -= 0.05f;
                if (params->lfo.depth < 0.0f) params->lfo.depth = 0.0f;
            } else if (ch == 'w') {  
                params->osc.wave_type = (params->osc.wave_type + 1) % 3;
            } else if (ch == 'e') { 
                params->lfo.wave_type = (params->lfo.wave_type + 1) % 3;
            } else if (ch == 'b') {  
                if (params->osc.num_voices > 1) params->osc.num_voices -= 1;
            } else if (ch == 'n') { 
                if (params->osc.num_voices < MAX_VOICES) params->osc.num_voices += 1;
            }
        }

        // Clear screen
        printf("\033[2J\033[H");

        // Print parameters with controls
        printf("C-EMBEDDED-SYNTH PARAMETERS\n");
        printf("---------------------------\n");
        printf("Oscillator Frequency: %.2f Hz    (g: increase, h: decrease)\n", params->osc.base_freq);
        printf("LFO Frequency:        %.2f Hz    (j: increase, k: decrease)\n", params->lfo.base_freq);
        printf("LFO Depth:            %.2f       (d: increase, f: decrease)\n", params->lfo.depth);
        printf("--------------------------------------------------------------------\n");
        printf("Oscillator Waveform:  %s         (w: cycle through waveforms)\n", 
                params->osc.wave_type == WAVE_SIN ? "Sine" : 
                params->osc.wave_type == WAVE_SAW ? "Sawtooth" : "Square");
        printf("LFO Waveform:         %s         (e: cycle through waveforms)\n",
                params->lfo.wave_type == WAVE_SIN ? "Sine" : 
                params->lfo.wave_type == WAVE_SAW ? "Sawtooth" : "Square");
        printf("Voices:               %.d       (n: increase, b: decrease)\n", params->osc.num_voices);

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
        
        // main
        float frequencyMod = 1.0f + (lfoValue * lfo->depth);
        // voices
        float currentVoiceIncrements[MAX_VOICES];
        for (int k = 0; k < osc->num_voices; k ++) { 
            currentVoiceIncrements[k] = osc->freqs[k] / SAMPLE_RATE * frequencyMod;
        }
        
        // Generate oscillator output using the modulated phase increment.
        switch (osc->wave_type) {
            case WAVE_SIN: {
                for (int k = 0; k < osc->num_voices; k ++) { 
                    int index = (int)(osc->phases[k] * TABLE_SIZE) % TABLE_SIZE;
                    sample += SINELUT[index];
                }
                sample /= (float)osc->num_voices;
                break;
            }
            case WAVE_SAW:
                for (int k = 0; k < osc->num_voices; k ++) { 
                    sample += (2.0f * osc->phases[k] - 1.0f);
                }
                sample /= (float)osc->num_voices;
                break;
            case WAVE_SQU:
                for (int k = 0; k < osc->num_voices; k ++) { 
                    sample += (osc->phase < 0.5f) ? -1.0f: 1.0f;
                }
                sample /= (float)osc->num_voices;
                break;
            default: {
                for (int k = 0; k < osc->num_voices; k ++) { 
                    int index = (int)(osc->phases[k] * TABLE_SIZE) % TABLE_SIZE;
                    sample += SINELUT[index];
                }
                sample /= (float)osc->num_voices;
                break;
            }
        }
        
        // Write stereo sample.
        *out++ = sample;
        *out++ = sample;

        //voices
        for (int k = 0; k < osc->num_voices; k ++) { 
            osc->phases[k] += currentVoiceIncrements[k];
            if (osc->phases[k] >= 1.0f)
                osc->phases[k] -= 1.0f;
        }

        // Update LFO phase.
        lfo->phase += lfo->base_freq / SAMPLE_RATE;
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
    params.osc.base_freq = 240.0f;
    params.osc.phase = 0.0f;
    params.osc.wave_type = WAVE_SIN;
    params.osc.num_voices = 3;
    for (int k = 0; k < MAX_VOICES; k++) { 
        params.osc.freqs[k] = params.osc.base_freq * (0.99f + ((float)rand() / RAND_MAX) * 0.01f); // random variation between 90% and 110% of base base_freq
        params.osc.phases[k] = 0.0f;
    } 
    
    params.lfo.depth = 0.2f;
    params.lfo.base_freq = 10.0f;
    params.lfo.phase = 0.0f;
    params.lfo.wave_type = WAVE_SAW;  // You can change this to WAVE_SIN or WAVE_SQU.
    
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
    // On Linux, start the input thread to adjust LFO base_freq.
    pthread_t thread;
    if (pthread_create(&thread, NULL, input_thread, &params) != 0) {
        fprintf(stderr, "Error creating input thread.\n");
        return -1;
    }
    sleep(100);
    pthread_cancel(thread);
#else
    while (1) {
        embedded_delay_ms(10);
    }
#endif
    
    ma_device_uninit(&device);
    return 0;
}

#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Define a structure to hold oscillator state.
typedef struct {
    float phase;
    float phaseIncrement;
} oscillator;

// This callback is called when the audio device needs more data.
void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    oscillator* osc = (oscillator*)pDevice->pUserData;
    float* out = (float*)pOutput;
    for (ma_uint32 i = 0; i < frameCount; i++) {
        // Generate a sine wave sample.
        float sample = sinf(osc->phase);
        
        // Update the phase. Wrap around when reaching 2*PI.
        osc->phase += osc->phaseIncrement;
        if (osc->phase >= 2.0f * 3.14159265f) {
            osc->phase -= 2.0f * 3.14159265f;
        }
        
        // Output the same sample to both channels (stereo).
        *out++ = sample;  // Left channel.
        *out++ = sample;  // Right channel.
    }
}

int main() {
    // Initialize oscillator for a 440 Hz tone.
    oscillator osc;
    osc.phase = 0.0f;
    osc.phaseIncrement = (2.0f * 3.14159265f * 440.0f) / 48000.0f;  // 440 Hz tone with a sample rate of 48000 Hz.

    ma_device device;
    ma_device_config config = ma_device_config_init(ma_device_type_playback);
    config.playback.format   = ma_format_f32;  // Using 32-bit floating point samples.
    config.playback.channels = 2;              // Stereo output.
    config.sampleRate        = 48000;          // Sample rate.
    config.dataCallback      = data_callback;  // Callback for generating audio.
    config.pUserData         = &osc;           // Pass our oscillator state to the callback.

    if (ma_device_init(NULL, &config, &device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to initialize the audio device.\n");
        return -1;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        fprintf(stderr, "Failed to start the audio device.\n");
        ma_device_uninit(&device);
        return -1;
    }
    
    // Keep the program running so that audio can play.
    printf("Playing 440 Hz sine wave for 5 seconds...\n");
#ifdef _WIN32
    Sleep(5000);
#else
    sleep(5);
#endif

    ma_device_uninit(&device);
    return 0;
}


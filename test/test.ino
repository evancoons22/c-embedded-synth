#include <Audio.h>
#include <Wire.h>
#include <SPI.h>

AudioSynthWaveformSine   sine1;
AudioOutputI2S           i2s1;
AudioControlSGTL5000     codec;
AudioConnection          patchCordL(sine1, 0, i2s1, 0);
AudioConnection          patchCordR(sine1, 0, i2s1, 1);

void setup() {
  AudioMemory(8);

  codec.enable();              // power up codec
  codec.volume(0.70);          // overall volume 0-1.0
  codec.unmuteHeadphone();     // just in case

  sine1.frequency(440);        // A4
  sine1.amplitude(0.90);
}

void loop() { }                // DMA runs by itself


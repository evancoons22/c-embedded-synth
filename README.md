## Run program

```
gcc -lm main.c -o main && ./main
```

## Using these github repos and resources: 
1. [PaulStaffrogen/core](https://github.com/PaulStoffregen/cores)
2. [tsoding/nob.h](https://github.com/tsoding/nob.h)
3. [arm compiler](https://developer.arm.com/downloads/-/gnu-rm)

## commands
check memory regions: 
```
arm-none-eabi-objdump -h main.elf
```

check sizes used: 
```
arm-none-eabi-size main.elf
```

dump linker script from newlib: 
```
arm-none-eabi-gcc -mcpu=arm7tdmi -mthumb -Wl,-verbose -o main.elf main.c
```

convert elf to hex: 
```
arm-none-eabi-objcopy -O ihex main.elf main.hex
```



## Goals

- create a standalone sythesizer
- output to aux
- simple rules, filters, parameters that yield complex sounds

## Hardware
1. Microcontroller: **Teensy 4.1**
2. User Controls: 
    - **potentiators** (Flutesan 12 Pieces 10K Ohm Trim Potentiometer Kit with Knob Variable Resistors Breadboard Trimmer Potentiometer Assortment Kit Compatible with Arduino, Blue)
    - **buttons** (QTEATAK 20 Pcs 6mm 2 Pin Momentary Tactile Tact Push Button Switch Through Hole Breadboard Friendly for Panel PCB)
3. Audio Output: 
    - **DAC MCP4921** (Microchip MCP4921-E/P 12-Bit Voltage Output Digital-to-Analog Converter (Pack of 2))
4. Power Supply: 
    - **5V USB Supply to Teensy 4.1**
5. Breadboard: **standard breadboard**  (4PCS Breadboards Kit Include 2PCS 830 Point 2PCS 400 Point Solderless Breadboards for Proto Shield Distribution Connecting Blocks)

## Software
Miniaudio
- do I need to use minaudio if configuring the DAC is simple? 
- use for waveform generation and sending data to DAC. is waveform generation easy? 
- maybe use some custom DSP code.
- use efficient algorithms and fixed point math
- use a circular buffer that is a few milliseconds ahead to prevent underruns
- WHAT will miniaudio make much easier? 

## Other notes
- don't max out CPU. Indicate usage with lights.

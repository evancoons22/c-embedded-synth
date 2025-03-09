## Run program

```
cc -o nob nob.c  # only once!!
./nob host && ./main
```

## Using these github repos and resources: 
1. [PaulStaffrogen/core](https://github.com/PaulStoffregen/cores)
2. [tsoding/nob.h](https://github.com/tsoding/nob.h)
3. [arm compiler](https://developer.arm.com/downloads/-/gnu-rm)

## commands for compiling and errors
check memory regions: 
```
arm-none-eabi-objdump -h main.elf
```

check sizes of memory spaces: 
```
arm-none-eabi-size main.elf
```

dump linker script from newlib compiler (a lightweight compiler): 
```
arm-none-eabi-gcc -mcpu=arm7tdmi -mthumb -Wl,-verbose -o main.elf main.c
```

convert elf to hex for use on teensy4.1: 
```
arm-none-eabi-objcopy -O ihex main.elf main.hex
```

## Idea
- create a standalone sythesizer on Teensy4.1
- output to aux
- simple rules, filters, parameters with hardware

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

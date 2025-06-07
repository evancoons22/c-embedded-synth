// nob.c
#include <stdio.h>
#include <string.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [host|embedded|emb2]\n", argv[0]);
        return 1;
    }

    Nob_Cmd cmd = {0};

    if (strcmp(argv[1], "host") == 0) {
        nob_cmd_append(&cmd, "cc", "-lm", "-o", "main", "main.c");
    }
    else if (strcmp(argv[1], "embedded") == 0) { 
        nob_cmd_append(&cmd, "arm-none-eabi-g++",
                "-mcpu=cortex-m7", "-mthumb", "-O2", "-std=c++17",
                "-ffunction-sections", "-fdata-sections",
                "-DTEENSY", "-DEMBEDDED", "-D__IMXRT1062__", "-DARDUINO", "-DARDUINO_TEENSY40", "-DLAYOUT_US_ENGLISH",
                "-I/home/evan/tools/cores/teensy4",
                "-T./teensy41.ld", "-Wall", "-Wextra", "-Wno-unused-function",
                "-o", "synth.elf", "main2.c" 
                );  // Adjust path to Teensy core
    } else if (strcmp(argv[1], "emb2") == 0) { 
        //nob_cmd_append(&cmd, "arm-none-eabi-gcc", "-mcpu=arm7tdmi", "-mthumb",  "-O2",  "-specs=nosys.specs", "-o", "main.elf", "main3.c");
        nob_cmd_append(&cmd, "arm-none-eabi-gcc", "-mcpu=cortex-m7", "-mthumb", "-mfpu=fpv5-d16", "-mfloat-abi=hard", "-O2", "-w", "-specs=nosys.specs", "-o", "main.elf", "main3.c");
    } 
    // working compilation with newlib:
    // arm-none-eabi-gcc -mcpu=arm7tdmi -mthumb -O2 -specs=nosys.specs -o main.elf main3.c
    // another compilation effort
    // arm-none-eabi-gcc -mcpu=arm7tdmi -mthumb -O2 -T teensy41.ld -lgcc -specs=nosys.specs -o main.elf main3.c
    else {
        fprintf(stderr, "Unknown target: %s\n", argv[1]);
        return 1;
    }

    if (!nob_cmd_run_sync(cmd))
        return 1;

    return 0;
}


// nob.c
#include <stdio.h>
#include <string.h>
#define NOB_IMPLEMENTATION
#include "nob.h"

int main(int argc, char **argv)
{
    NOB_GO_REBUILD_URSELF(argc, argv);

    if (argc < 2) {
        fprintf(stderr, "Usage: %s [host|embedded]\n", argv[0]);
        return 1;
    }

    Nob_Cmd cmd = {0};

    if (strcmp(argv[1], "host") == 0) {
        nob_cmd_append(&cmd, "cc", "-lm", "-o", "main", "main.c");
    }
    //    else if (strcmp(argv[1], "embedded") == 0) {
    //        nob_cmd_append(&cmd, "arm-none-eabi-gcc",
    //            "-mcpu=cortex-m7", "-mthumb", "-O2", 
    //            "-ffunction-sections", "-fdata-sections",
    //            "-I./miniaudio.h", "-DTEENSY", "-DEMBEDDED",
    //            "-Tteensy41.ld", "-Wall", "-Wextra", "-Wno-unused-function",
    //            "-o", "synth.elf", "main.c");
    //    }
    else if (strcmp(argv[1], "embedded") == 0) { 
        nob_cmd_append(&cmd, "arm-none-eabi-g++",
                "-mcpu=cortex-m7", "-mthumb", "-O2", "-std=c++17",
                "-ffunction-sections", "-fdata-sections",
                "-DTEENSY", "-DEMBEDDED", "-D__IMXRT1062__", "-DARDUINO", "-DARDUINO_TEENSY40", "-DLAYOUT_US_ENGLISH",
                "-I/home/evan/tools/cores/teensy4",
                "-T./teensy41.ld", "-Wall", "-Wextra", "-Wno-unused-function",
                "-o", "synth.elf", "main2.c" 
                );  // Adjust path to Teensy core
    } 
    else {
        fprintf(stderr, "Unknown target: %s\n", argv[1]);
        return 1;
    }

    if (!nob_cmd_run_sync(cmd))
        return 1;

    return 0;
}


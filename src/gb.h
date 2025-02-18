#ifndef __GB_H__
#define __GB_H__

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "raylib.h"
#include "utils.h"
#include "mem.h"
#include "cpu.h"
#include "cartridge.h"


#ifdef __cplusplus
extern "C" {
#endif

extern FILE *doctorLogFile;


#define MAIN_CLOCK_HZ (4194304)
#define MAIN_CLOCKS_PER_CPU_CYCLE   (4)
#define CPU_CYCLE_HZ  (MAIN_CLOCK_HZ/MAIN_CLOCKS_PER_CPU_CYCLE)

void gbInit(const char * const cartFilename);
void gbDeinit(void);

void cpuCycles(int cycles);

// get/set just access the memory.  read/write trigger cpu cycles
uint8_t getMem8(uint16_t addr);
void setMem8(uint16_t addr, uint8_t val8);
uint8_t readMem8(uint16_t addr);
void writeMem8(uint16_t addr, uint8_t val8);
uint16_t readMem16(uint16_t addr);
void writeMem16(uint16_t addr, uint16_t val16);

#ifdef __cplusplus
}
#endif
#endif // __GB_H__

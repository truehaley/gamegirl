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
#include "serial.h"
#include "timer.h"
#include "graphics.h"
#include "controls.h"


#ifdef __cplusplus
extern "C" {
#endif

extern FILE *doctorLogFile;
extern bool serialConsole;
extern bool exitOnBreak;
extern bool running;


#define MAIN_CLOCK_HZ (4194304)
#define MAIN_CLOCKS_PER_CPU_CYCLE   (4)
#define CPU_CYCLE_HZ  (MAIN_CLOCK_HZ/MAIN_CLOCKS_PER_CPU_CYCLE)

// Serial Regs
#define REG_SB_ADDR     (0xFF01)
#define REG_SC_ADDR     (0xFF02)
// Timer Regs
#define REG_DIV_ADDR    (0xFF04)
#define REG_TIMA_ADDR   (0xFF05)
#define REG_TMA_ADDR    (0xFF06)
#define REG_TAC_ADDR    (0xFF07)
// Interrupt Regs
#define REG_IF_ADDR     (0xFF0F)
// LCD REGS
#define REG_LCDC_ADDR   (0xFF40)
#define REG_STAT_ADDR   (0xFF41)
#define REG_SCY_ADDR    (0xFF42)
#define REG_SCX_ADDR    (0xFF43)
#define REG_LY_ADDR     (0xFF44)
#define REG_LYC_ADDR    (0xFF45)
#define REG_BGP_ADDR    (0xFF47)
#define REG_OBP0_ADDR   (0xFF48)
#define REG_OBP1_ADDR   (0xFF49)
#define REG_WY_ADDR     (0xFF4A)
#define REG_WX_ADDR     (0xFF4B)
// Interrupt Regs
#define REG_IE_ADDR     (0xFFFF)

// TODO what do missing/unmapped regs return?
#define MISSING_REG_VAL  (0x00)
#define UNMAPPED_REG_VAL (0xFF)

void gbInit(const char * const cartFilename);
void gbDeinit(void);

void cpuCycle();
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

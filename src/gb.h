// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#ifndef __GB_H__
#define __GB_H__
// IWYU pragma: always_keep

// IWYU pragma: begin_exports
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

#include "raylib.h"
#include "utils.h"
#include "mem.h"
#include "cpu.h"
#include "cartridge.h"
#include "serial.h"
#include "timer.h"
#include "display.h"
#include "controls.h"
#include "audio.h"
// IWYU pragma: end_exports


#ifdef __cplusplus
extern "C" {
#endif

extern FILE *doctorLogFile;
extern bool serialConsole;
extern bool exitOnBreak;
extern bool running;
extern bool fastBoot;
extern bool mooneye;
extern bool bootRomActive;

#define MAIN_CLOCK_HZ (4194304)
#define MAIN_CLOCKS_PER_CPU_CYCLE   (4)
#define CPU_CYCLE_HZ  (MAIN_CLOCK_HZ/MAIN_CLOCKS_PER_CPU_CYCLE)

// Joypad Regs
#define REG_JOYP_ADDR   (0xFF00)
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

#define REG_AUD_CH1_SWEEP (0xFF10)
#define REG_AUD_CH1_TIMER (0xFF11)
#define REG_AUD_CH1_ENVLP (0xFF12)
#define REG_AUD_CH1_LPER  (0xFF13)
#define REG_AUD_CH1_CTRL  (0xFF14)
#define REG_AUD_CH2_TIMER (0xFF16)
#define REG_AUD_CH2_ENVLP (0xFF17)
#define REG_AUD_CH2_LPER  (0xFF18)
#define REG_AUD_CH2_CTRL  (0xFF19)
#define REG_AUD_CH3_DAC   (0xFF1A)
#define REG_AUD_CH3_TIMER (0xFF1B)
#define REG_AUD_CH3_ENVLP (0xFF1C)
#define REG_AUD_CH3_LPER  (0xFF1D)
#define REG_AUD_CH3_CTRL  (0xFF1E)
#define REG_AUD_CH4_TIMER (0xFF20)
#define REG_AUD_CH4_ENVLP (0xFF21)
#define REG_AUD_CH4_LFSR  (0xFF22)
#define REG_AUD_CH4_CTRL  (0xFF23)
#define REG_AUD_MAST_VOL  (0xFF24)
#define REG_AUD_MAST_PAN  (0xFF25)
#define REG_AUD_MAST_CTRL (0xFF26)
#define REG_AUD_CH3_WAV0  (0xFF30)
#define REG_AUD_CH3_WAV1  (0xFF31)
#define REG_AUD_CH3_WAV2  (0xFF32)
#define REG_AUD_CH3_WAV3  (0xFF33)
#define REG_AUD_CH3_WAV4  (0xFF34)
#define REG_AUD_CH3_WAV5  (0xFF35)
#define REG_AUD_CH3_WAV6  (0xFF36)
#define REG_AUD_CH3_WAV7  (0xFF37)
#define REG_AUD_CH3_WAV8  (0xFF38)
#define REG_AUD_CH3_WAV9  (0xFF39)
#define REG_AUD_CH3_WAVA  (0xFF3A)
#define REG_AUD_CH3_WAVB  (0xFF3B)
#define REG_AUD_CH3_WAVC  (0xFF3C)
#define REG_AUD_CH3_WAVD  (0xFF3D)
#define REG_AUD_CH3_WAVE  (0xFF3E)
#define REG_AUD_CH3_WAVF  (0xFF3F)


// LCD REGS
#define REG_LCDC_ADDR   (0xFF40)
#define REG_STAT_ADDR   (0xFF41)
#define REG_SCY_ADDR    (0xFF42)
#define REG_SCX_ADDR    (0xFF43)
#define REG_LY_ADDR     (0xFF44)
#define REG_LYC_ADDR    (0xFF45)
#define REG_OAM_ADDR    (0xFF46)
#define REG_BGP_ADDR    (0xFF47)
#define REG_OBP0_ADDR   (0xFF48)
#define REG_OBP1_ADDR   (0xFF49)
#define REG_WY_ADDR     (0xFF4A)
#define REG_WX_ADDR     (0xFF4B)
// Interrupt Regs
#define REG_IE_ADDR     (0xFFFF)

// TODO what do missing/unmapped regs return?
#define MISSING_REG_VAL  (0xFF)
#define UNMAPPED_REG_VAL (0xFF)

void gbInit(const char * const cartFilename);
void gbDeinit(void);

void cpuCycle();
void cpuCycles(int cycles);

// get/set just access the memory.  read/write trigger cpu cycles
uint8_t getMem8(uint16_t addr);
uint8_t getRawMem8(uint16_t addr);
void setMem8(uint16_t addr, uint8_t val8);
uint8_t readMem8(uint16_t addr);
void writeMem8(uint16_t addr, uint8_t val8);
uint16_t readMem16(uint16_t addr);
void writeMem16(uint16_t addr, uint16_t val16);

#ifdef __cplusplus
}
#endif
#endif // __GB_H__

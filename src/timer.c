// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#include "gb.h"

// https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html
// https://hacktix.github.io/GBEDG/timers/

static struct {
    union {
        uint16_t full;
        struct {
            uint16_t lower:6;
            uint16_t val:8;
            uint16_t :2;
        };
        struct {
            uint16_t :1;
            uint16_t clk1:1;
            uint16_t :1;
            uint16_t clk2:1;
            uint16_t :1;
            uint16_t clk3:1;
            uint16_t :1;
            uint16_t clk0:1;
            uint16_t x:8;
        };
    } DIV;  // FF04
    struct {
        uint8_t val;
    } TIMA; // FF05
    struct {
        uint8_t val;
    } TMA;  // FF06
    struct {
        uint8_t val;
    } TMA_new;  // FF06
    union {
        uint8_t val;
        struct {
            uint8_t clkSel:2;
            uint8_t en:1;
            uint8_t reserved:5;
        };
    } TAC;  /// FF07
} regs;

#define TAC_CLK0_MASK   (0x0080)
#define TAC_CLK1_MASK   (0x0002)
#define TAC_CLK2_MASK   (0x0008)
#define TAC_CLK3_MASK   (0x0020)
uint16_t timerClkMask = TAC_CLK0_MASK;

static bool overflowHappened = false;
static bool timaUpdated = false;

static void incrementTIMA(void)
{
    // Note: Handling of the TAC.en bit should already be factored in prior to calling this function

    // TIMA overflows is technically detected by seeing a falling edge on bit 7.
    // As a result, TIMA will read as zero for a cycle before the new value is loaded from TMA
    //   similarly, the interrupt files one cycle after the overflow at the same time that TMA is loaded.
    // https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html

    // Note: post-increment allows us to test for overflow when the value is 0xFF beforehand
    if(0xFF == regs.TIMA.val++) {
        overflowHappened = true;
    }
}


void setTimerReg8(uint16_t addr, uint8_t val8)
{
    // See See https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html for expanations of how extra TIMA
    //  increments may happen
    if( REG_DIV_ADDR == addr ) {
        // Due to the way the hardware works, resetting DIV to zero could cause TIMA to increment if the
        //  chosen TIMA clock signal would transition from high to low
        if( (1 == regs.TAC.en) && (0 != (regs.DIV.full & timerClkMask)) ) {
            incrementTIMA();
        }
        regs.DIV.full = 0;
    } else if( REG_TMA_ADDR == addr ) {
        regs.TMA.val = val8;
        // https://hacktix.github.io/GBEDG/timers/
        // If TIMA was updated due to an overflow in this same cycle, the new TMA
        //  value is effectively written-through
        if(timaUpdated) {
            regs.TIMA.val = val8;
        }
    } else if( REG_TAC_ADDR == addr ) {
        // Note that due to the way the hardware works, writing to this register may increase TIMA once
        //  if the clock signal makes a high to low transition.
        const uint16_t timerClockBefore = (1 == regs.TAC.en)? (regs.DIV.full & timerClkMask) : 0;
        regs.TAC.val = val8;
        switch(regs.TAC.clkSel) {
            case 0: { timerClkMask = TAC_CLK0_MASK; break; }
            case 1: { timerClkMask = TAC_CLK1_MASK; break; }
            case 2: { timerClkMask = TAC_CLK2_MASK; break; }
            case 3: { timerClkMask = TAC_CLK3_MASK; break; }
        }
        const uint16_t timerClockAfter = (1 == regs.TAC.en)? (regs.DIV.full & timerClkMask) : 0;
        // test for the high to low transition
        if( (0 != timerClockBefore) && (0 == timerClockAfter) ) {
            incrementTIMA();
        }

    } else if( REG_TIMA_ADDR == addr ) {
        // https://hacktix.github.io/GBEDG/timers/
        // cancels any pending overflow that happens this same cycle
        overflowHappened = false;
        // however, if the overflow just happened then the value from TMA takes precedence
        //   and this write to TIMA is effectively ignored
        if(!timaUpdated) {
            regs.TIMA.val = val8;
        }
    }
}

uint8_t getTimerReg8(uint16_t addr)
{
    if( REG_DIV_ADDR == addr ) {
        return regs.DIV.val;
    } else if( REG_TIMA_ADDR == addr ) {
        return regs.TIMA.val;
    } else if( REG_TMA_ADDR == addr ) {
        return regs.TMA.val;
    } else if( REG_TAC_ADDR == addr ) {
        regs.TAC.reserved = 0x1F;   // reads as 1s
        return regs.TAC.val;
    }
    return 0x00;
}

void timerTick(void)
{
    // See See https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html for expanations of how
    //  the TIMA increments and interrupt triggers actually work

    if( cpuStopped() ) {
        // TODO: should we check for spurious TIMA increments here as well?
        regs.DIV.full = 0;
        return;
    }

    timaUpdated = false;

    // The pending interrupt and TMA reload is handled 1 cycle after the actual overflow
    if(overflowHappened) {
        overflowHappened = false;
        regs.TIMA.val = regs.TMA.val;
        setIntFlag(INT_TIMER);
        // If a write to TIMA happens this same cycle, it will be ignored
        //  however, if TMA is written this same cycle TIMA will reflect the new value
        timaUpdated = true;
    }

    const uint16_t timerClockBefore = (regs.DIV.full & timerClkMask);
    regs.DIV.full++;
    const uint16_t timerClockAfter = (regs.DIV.full & timerClkMask);

    if( (0 != timerClockBefore) && (0 == timerClockAfter) && (1 == regs.TAC.en) ) {
        incrementTIMA();
    }
}

void timerInit(void)
{
    memset(&regs, 0, sizeof(regs));
    timerClkMask = TAC_CLK0_MASK;
    overflowHappened = false;
    timaUpdated = false;
}

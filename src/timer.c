#include "cpu.h"
#include "gb.h"

// TODO: https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html

static struct {
    struct {
        uint8_t val;
    } DIV;  // FF04
    struct {
        uint8_t val;
    } TIMA; // FF05
    struct {
        uint8_t val;
    } TMA;  // FF06
    struct {
        union {
            uint8_t val;
            struct {
                uint8_t clkSel:2;
                uint8_t en:1;
                uint8_t reserved:5;
            };
        };
    } TAC;  /// FF07
} regs;

const int divCycleReset = 64;
int divCycleCount = 0;
int tacCycleReset = 256;
int tacCycleCount = 0;

void setTimerReg8(uint16_t addr, uint8_t val8)
{

    if( REG_DIV_ADDR == addr ) {
        divCycleCount = divCycleReset;
        tacCycleCount = tacCycleReset;
        regs.DIV.val = 0;
    } else if( REG_TMA_ADDR == addr ) {
        // TODO: If a TMA write is executed on the same M-cycle as the content of TMA is
        // transferred to TIMA due to a timer overflow, the old value is transferred to TIMA.
        regs.TMA.val = val8;
    } else if( REG_TAC_ADDR == addr ) {
        // TODO: Note that writing to this register may increase TIMA once
        regs.TAC.val = val8;
        switch(regs.TAC.clkSel) {
            case 0: { tacCycleReset = 256; break; }
            case 1: { tacCycleReset = 4; break; }
            case 2: { tacCycleReset = 16; break; }
            case 3: { tacCycleReset = 64; break; }
        }
    } else if( REG_TIMA_ADDR == addr ) {
        regs.TIMA.val = val8;
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
        return regs.TAC.val;
    }
    return 0x00;
}

void timerTick(void)
{
    static bool intPending = false;

    if( cpuStopped() ) {
        divCycleCount = divCycleReset;
        tacCycleCount = tacCycleReset;
        regs.DIV.val = 0;
        return;
    }

    if( 0 == divCycleCount-- ) {
        divCycleCount = divCycleReset;
        regs.DIV.val++;
    }

    if(intPending) {
        setIntFlag(INT_TIMER);
        intPending = false;
    }
    if( 0 == tacCycleCount-- ) {
        tacCycleCount = tacCycleReset;

        if( regs.TAC.en ) {
            // TODO: When TIMA overflows, the value from TMA is copied, and the timer flag is set in IF,
            // but one M-cycle later. This means that TIMA is equal to $00 for the M-cycle after it overflows.
            // ...So we test for 0x00 rather than 0xFF to detect the overflow
            if(0xFF == regs.TIMA.val++) {
                // overflow
                regs.TIMA.val = regs.TMA.val;
                // TODO: If a TMA write is executed on the same M-cycle as the content of TMA is
                // transferred to TIMA due to a timer overflow, the old value is transferred to TIMA.

                intPending = true;
            }
        }
    }
}

void timerInit(void)
{
    memset(&regs, 0, sizeof(regs));
    //regs.tac.reserved = 0x1F;
    divCycleCount = divCycleReset;
    tacCycleReset = 256;
    tacCycleCount = tacCycleReset;
}

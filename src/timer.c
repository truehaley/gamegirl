#include "cpu.h"
#include "gb.h"

// TODO: https://gbdev.io/pandocs/Timer_Obscure_Behaviour.html

static struct {
    struct {
        uint8_t val;
    } div;
    struct {
        uint8_t val;
    } tima;
    struct {
        uint8_t val;
    } tma;
    struct {
        union {
            uint8_t val;
            struct {
                uint8_t clkSel:2;
                uint8_t en:1;
                uint8_t reserved:5;
            };
        };
    } tac;
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
        regs.div.val = 0;
    } else if( REG_TMA_ADDR == addr ) {
        // TODO: If a TMA write is executed on the same M-cycle as the content of TMA is
        // transferred to TIMA due to a timer overflow, the old value is transferred to TIMA.
        regs.tma.val = val8;
    } else if( REG_TAC_ADDR == addr ) {
        // TODO: Note that writing to this register may increase TIMA once
        regs.tac.val = val8;
        //regs.tac.reserved = 0x1F;
        switch(regs.tac.clkSel) {
            case 0: { tacCycleReset = 256; break; }
            case 1: { tacCycleReset = 4; break; }
            case 2: { tacCycleReset = 16; break; }
            case 3: { tacCycleReset = 64; break; }
        }
    } else if( REG_TIMA_ADDR == addr ) {
        regs.tima.val = val8;
    }
}

uint8_t getTimerReg8(uint16_t addr)
{
    if( REG_DIV_ADDR == addr ) {
        return regs.div.val;
    } else if( REG_TIMA_ADDR == addr ) {
        return regs.tima.val;
    } else if( REG_TMA_ADDR == addr ) {
        return regs.tma.val;
    } else if( REG_TAC_ADDR == addr ) {
        return regs.tac.val;
    }
    return 0x00;
}

void timerTick(void)
{
    static bool intPending = false;

    if( cpuStopped() ) {
        divCycleCount = divCycleReset;
        tacCycleCount = tacCycleReset;
        regs.div.val = 0;
        return;
    }

    if( 0 == divCycleCount-- ) {
        divCycleCount = divCycleReset;
        regs.div.val++;
    }

    if(intPending) {
        setIntFlag(INT_TIMER);
        intPending = false;
    }
    if( 0 == tacCycleCount-- ) {
        tacCycleCount = tacCycleReset;

        if( regs.tac.en ) {
            // TODO: When TIMA overflows, the value from TMA is copied, and the timer flag is set in IF,
            // but one M-cycle later. This means that TIMA is equal to $00 for the M-cycle after it overflows.
            // ...So we test for 0x00 rather than 0xFF to detect the overflow
            if(0xFF == regs.tima.val++) {
                // overflow
                regs.tima.val = regs.tma.val;
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

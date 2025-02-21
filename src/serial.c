#include "gb.h"

struct {
    struct {
        uint8_t val;
    } SB;
    struct {
        union {
            uint8_t val;
            struct {
                uint8_t clockSel:1;
                uint8_t clockSpeed:1;
                uint8_t reserved:5;
                uint8_t transfer:1;
            };
        };
    } SC;
} regs;

uint8_t getSerialReg8(uint16_t addr)
{
    if( REG_SB_ADDR == addr ) {
        return regs.SB.val;
    } else if (REG_SC_ADDR == addr) {
        if( serialConsole ) {
            return 0xFF;
        } else {
            return regs.SC.val;
        }
    }
    return MISSING_REG_VAL;
}

void setSerialReg8(uint16_t addr, uint8_t val8)
{
    if( REG_SB_ADDR == addr ) {
        regs.SB.val = val8;
    } else if (REG_SC_ADDR == addr) {
        regs.SC.val = val8;
        if(serialConsole) {
            if( 1 == regs.SC.transfer ) {
                printf("%c", regs.SB.val);
                fflush(stdout);
            }
        }
    }
}

void serialInit(void)
{

}

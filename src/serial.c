// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

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
        return 0; //regs.SB.val;
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
    memset(&regs, 0, sizeof(regs));
    regs.SC.val = 0x7E;
}

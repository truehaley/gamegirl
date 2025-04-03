// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#include "gb.h"
#include "gui.h"

typedef struct __attribute__((packed)) {
    union {
        uint8_t val;
        union {
            struct {
                uint8_t readState:4;
                uint8_t pollSelect:2;
                uint8_t reserved:2;
            };
            struct {
                uint8_t btnA_padR:1;
                uint8_t btnB_padL:1;
                uint8_t sel_padU:1;
                uint8_t start_padD:1;
                uint8_t poll_direction:1;
                uint8_t poll_action:1;
                uint8_t reserved2:2;
            };
        };
    };
} JOYPReg;

struct {
    JOYPReg JOYP;
} regs;

ControlState rawControls;
ControlState activeControls;

uint8_t getControlsReg8(uint16_t addr)
{
    if( REG_JOYP_ADDR == addr ) {
        regs.JOYP.reserved = 0x3;  // reads as 1s

        // clear any action bits if selected
        regs.JOYP.readState = 0x0F;
        if( 0 == regs.JOYP.poll_action ) {
            regs.JOYP.readState &= ~activeControls.action;
        }
        // also clear any direction bits if selected (logic-or)
        if( 0 == regs.JOYP.poll_direction ) {
            regs.JOYP.readState &= ~activeControls.direction;
        }
        return regs.JOYP.val;
    }
    return MISSING_REG_VAL;
}

void setControlsReg8(uint16_t addr, uint8_t val8)
{
    JOYPReg temp;
    temp.val = val8;
    if( REG_JOYP_ADDR == addr ) {
        regs.JOYP.pollSelect = temp.pollSelect;
        // Other bits are not writeable
    }
}

void updateControls(ControlState newControls)
{
    ControlState changed;
    // which controls have changed since last time?
    changed.val = newControls.val ^ rawControls.val;
    // If the action buttons are selcted, one of them changed, and it was pressed, trigger interrupt
    if( (0 == regs.JOYP.poll_action) && (0 != changed.action) && (0 != (changed.action & newControls.action)) ) {
        setIntFlag(INT_JOYPAD);
    }
    // similar test for direction
    if( (0 == regs.JOYP.poll_direction) && (0 != changed.direction) && (0 != (changed.direction & newControls.direction)) ) {
        setIntFlag(INT_JOYPAD);
    }
    // TODO: reality is slightly more complex if both polling options are chosen...
    // This interrupt is useful to identify button presses if we have only selected either action (bit 5)
    //   or direction (bit 4), but not both. If both are selected and, for example, a bit is already held
    //   Low by an action button, pressing the corresponding direction button would make no difference.

    // TODO: If up/down and left/right should be exclusive... the newly pressed key should disable the one
    //   still being held down, and on release whichever is still held down should be triggered
    rawControls = newControls;
    activeControls = newControls;
}

Vector2 guiDrawControls(const Vector2 viewAnchor)
{
    // Top left corner of the controls interface
    Vector2 anchor = viewAnchor;

    // D-pad horiz
    DrawRectangle(viewAnchor.x+30, viewAnchor.y+10 + 30, 90, 30, DARKGRAY);
    if( 1 == activeControls.dpadLeft ) {
        DrawRectangle(viewAnchor.x+30 +2, viewAnchor.y+10 + 32, 30-4, 30-4, LIGHTGRAY);
    }
    if( 1 == activeControls.dpadRight ) {
        DrawRectangle(viewAnchor.x+30 +62, viewAnchor.y+10 + 32, 30-4, 30-4, LIGHTGRAY);
    }

    // D-pad vert
    DrawRectangle(viewAnchor.x+30 + 30, viewAnchor.y+10, 30, 90, DARKGRAY);
    if( 1 == activeControls.dpadUp ) {
        DrawRectangle(viewAnchor.x+30 + 32, viewAnchor.y+10 +2, 30-4, 30-4, LIGHTGRAY);
    }
    if( 1 == activeControls.dpadDown ) {
        DrawRectangle(viewAnchor.x+30 + 32, viewAnchor.y+10 +62, 30-4, 30-4, LIGHTGRAY);
    }

    // B
    DrawCircle(viewAnchor.x + 370, viewAnchor.y + 70, 22, MAROON);
    if( 1 == activeControls.buttonB ) {
        DrawCircle(viewAnchor.x + 370, viewAnchor.y + 70, 22-3, RED);
    }

    // A
    DrawCircle(viewAnchor.x + 430, viewAnchor.y + 45, 22, MAROON);
    if( 1 == activeControls.buttonA ) {
        DrawCircle(viewAnchor.x + 430, viewAnchor.y + 45, 22-3, RED);
    }

    // Select
    DrawRectangle(viewAnchor.x + 170, viewAnchor.y + 100, 50, 15, GRAY);
    if( 1 == activeControls.select ) {
        DrawRectangle(viewAnchor.x + 170 +2, viewAnchor.y + 100 +2, 50-4, 15-4, LIGHTGRAY);
    }

    // Start
    DrawRectangle(viewAnchor.x + 250, viewAnchor.y + 100, 50, 15, GRAY);
    if( 1 == activeControls.start ) {
        DrawRectangle(viewAnchor.x + 250 +2, viewAnchor.y + 100 +2, 50-4, 15-4, LIGHTGRAY);
    }

    return (Vector2){ SCREEN_WIDTH*3, 125 };
}

void controlsInit(void)
{
    regs.JOYP.val = 0xCF;   // reset val
}

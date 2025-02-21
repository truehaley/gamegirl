#include "gb.h"
#include "gui.h"

typedef struct __attribute__((packed)) {
    union {
        uint8_t val;
        struct {
            union {
                uint8_t readState:4;
                struct {
                    uint8_t btnA_padR:1;
                    uint8_t btnB_padL:1;
                    uint8_t sel_padU:1;
                    uint8_t start_padD:1;
                };
            };
            union {
                uint8_t stateSelect:2;
                struct {
                    uint8_t poll_dpad:1;
                    uint8_t poll_buttons:1;
                };
            };
            uint8_t reserved:2;
        };
    };
} JOYPReg;

struct {
    JOYPReg JOYP;
} regs;

struct {
    uint8_t buttonA:1;
    uint8_t buttonB:1;
    uint8_t select:1;
    uint8_t start:1;
    uint8_t dpadRight:1;
    uint8_t dpadLeft:1;
    uint8_t dpadUp:1;
    uint8_t dpadDown:1;

} ControlState;

uint8_t getControlsReg8(uint16_t addr)
{
    if( REG_JOYP_ADDR == addr ) {
        regs.JOYP.readState = 0xF;  // TODO support actual controls
        return regs.JOYP.val;
    }
    return MISSING_REG_VAL;
}

void setControlsReg8(uint16_t addr, uint8_t val8)
{
    JOYPReg temp;
    temp.val = val8;
    if( REG_JOYP_ADDR == addr ) {
        regs.JOYP.stateSelect = temp.stateSelect;
    }
}

void guiDrawControls(void)
{
    // Top left corner of the controls interface
    Vector2 viewAnchor = { 10, 144*3 + 20 };
    Vector2 anchor = viewAnchor;

    DrawRectangle(viewAnchor.x+30, viewAnchor.y+10 + 30, 90, 30, DARKGRAY);   // D-pad horiz
    DrawRectangle(viewAnchor.x+30 + 30, viewAnchor.y+10, 30, 90, DARKGRAY);   // D-pad vert
    DrawCircle(viewAnchor.x + 370, viewAnchor.y + 70, 22, MAROON);      // B
    DrawCircle(viewAnchor.x + 430, viewAnchor.y + 45, 22, MAROON);      // A
    DrawRectangle(viewAnchor.x + 170, viewAnchor.y + 100, 50, 15, LIGHTGRAY);    // Select
    DrawRectangle(viewAnchor.x + 250, viewAnchor.y + 100, 50, 15, LIGHTGRAY);    // Start
}

void controlsInit(void)
{
    regs.JOYP.val = 0xFF;
}

#ifndef __CONTROLS_H__
#define __CONTROLS_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed)) {
    union {
        uint8_t val;
        union {
            struct {
                uint8_t action:4;
                uint8_t direction:4;
            };
            struct {
                uint8_t buttonA:1;
                uint8_t buttonB:1;
                uint8_t select:1;
                uint8_t start:1;
                uint8_t dpadRight:1;
                uint8_t dpadLeft:1;
                uint8_t dpadUp:1;
                uint8_t dpadDown:1;
            };
        };
    };
} ControlState;

uint8_t getControlsReg8(uint16_t addr);
void setControlsReg8(uint16_t addr, uint8_t val8);
void updateControls(ControlState newControls);
void guiDrawControls(void);
void controlsInit(void);


#ifdef __cplusplus
}
#endif

#endif //__CONTROLS_H__

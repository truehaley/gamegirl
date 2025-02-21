#ifndef __CONTROLS_H__
#define __CONTROLS_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8_t getControlsReg8(uint16_t addr);
void setControlsReg8(uint16_t addr, uint8_t val8);

void guiDrawControls(void);
void controlsInit(void);


#ifdef __cplusplus
}
#endif

#endif //__CONTROLS_H__

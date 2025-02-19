#ifndef __GRAPHICS_H__
#define __GRAPHICS_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GBCOL_WHITE     (0)
#define GBCOL_LIGHTGRAY (1)
#define GBCOL_DARKGRAY  (2)
#define GBCOL_BLACK     (3)

void setGfxReg8(uint16_t addr, uint8_t val8);
uint8_t getGfxReg8(uint16_t addr);

void ppuCycles(int dots);

void graphicsInit(void);
void guiDrawGraphics(void);


#ifdef __cplusplus
}
#endif

#endif //__GRAPHICS_H__

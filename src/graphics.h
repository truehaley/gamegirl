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

extern bool guiUpdateScreen;

void setGfxReg8(uint16_t addr, uint8_t val8);
uint8_t getGfxReg8(uint16_t addr);

void ppuCycles(int dots);
void oamDmaCycle(void);

void setVram8(uint16_t addr, uint8_t val8);
uint8_t getVram8(uint16_t addr);
void setOam8(uint16_t addr, uint8_t val8);
uint8_t getOam8(uint16_t addr);



void graphicsInit(void);
void graphicsDeinit(void);
void guiDrawGraphics(void);


#ifdef __cplusplus
}
#endif

#endif //__GRAPHICS_H__

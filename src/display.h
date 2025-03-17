#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#include "gb_types.h"

#define SCREEN_WIDTH    (160)
#define SCREEN_HEIGHT   (144)

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



void displayInit(void);
void displayDeinit(void);



Vector2 guiDrawDisplayObjects(const Vector2 anchor);
Vector2 guiDrawDisplayTileMap(const Vector2 anchor, const uint8_t map);
Vector2 guiDrawDisplayTileData(const Vector2 anchor);
Vector2 guiDrawDisplayScreen(const Vector2 anchor);
Vector2 guiDrawDisplay(const Vector2 anchor);


#ifdef __cplusplus
}
#endif

#endif //__DISPLAY_H__

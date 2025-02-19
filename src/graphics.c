#include "cpu.h"
#include "gb.h"
#include "gui.h"
#include "raygui.h"
#include "raylib.h"

typedef struct {
    union {
        uint8_t val;
        struct {
            uint8_t palCol0:2;
            uint8_t palCol1:2;
            uint8_t palCol2:2;
            uint8_t palCol3:2;
        };
    };
} PaletteReg;

struct {
    struct {
        uint8_t val;
    } WX;  // FF4B

    struct {
        uint8_t val;
    } WY;   // FF4A

    PaletteReg OBP1;    // FF49
    PaletteReg OBP0;    // FF48
    PaletteReg BGP;     // FF47

    struct {
        uint8_t val;
    } LYC;  // FF45

    struct {
        uint8_t val;
    } LY;   // FF44

    struct {
        uint8_t val;
    } SCX;  // FF43

    struct {
        uint8_t val;
    } SCY;   // FF42

    struct {
        union {
            uint8_t val;
            struct {
                uint8_t ppuMode:2;
                uint8_t lycMatch:1;
                uint8_t hblankInt0Enable:1;
                uint8_t vblankInt1Enable:1;
                uint8_t oamInt2Enable:1;
                uint8_t lycIntEnable:1;
                uint8_t reserved:1;
            };
        };
    } STAT; // FF41

    struct {
        union {
            uint8_t val;
            struct {
                uint8_t bgWinEnable:1;      // 0 = Off; 1 = On
                uint8_t objEnable:1;        // 0 = Off; 1 = On
                uint8_t objSize:1;          // 0 = 8×8; 1 = 8×16
                uint8_t bgTileMap:1;        // 0 = 9800–9BFF; 1 = 9C00–9FFF
                uint8_t bgWinTileData:1;    // 0 = 8800–97FF (signed); 1 = 8000–8FFF (unsigned)
                uint8_t windowEnable:1;     // 0 = Off; 1 = On
                uint8_t windowTileMap:1;    // 0 = 9800–9BFF; 1 = 9C00–9FFF
                uint8_t graphicsEnable:1;   // 0 = Off; 1 = On
            };
        };
    } LCDC;  // FF40
} regs;

#define INT_STAT_HBLANK (0x08)
#define INT_STAT_VBLANK (0x10)
#define INT_STAT_OAM    (0x20)
#define INT_STAT_LYC    (0x40)
uint8_t activeStatFlags = 0;

void maybeTriggerStatInterrupt(uint8_t newFlag)
{
    /*   https://gbdev.io/pandocs/Interrupt_Sources.html#int-48--stat-interrupt
    The various STAT interrupt sources (modes 0-2 and LYC=LY) have their state (inactive=low and active=high) logically ORed into a shared “STAT interrupt line” if their respective enable bit is turned on.

    A STAT interrupt will be triggered by a rising edge (transition from low to high) on the STAT interrupt line.

    If a STAT interrupt source logically ORs the interrupt line high while (or immediately after) it’s already set high by another source, then there will be no low-to-high transition and so no interrupt will occur. This phenomenon is known as “STAT blocking” (test ROM example).

    As mentioned in the description of the STAT register, the PPU cycles through the different modes in a fixed order. So for example, if interrupts are enabled for two consecutive modes such as Mode 0 and Mode 1, then no interrupt will trigger for Mode 1 (since the STAT interrupt line won’t have a chance to go low between them).
    */
    if( 0 == (activeStatFlags & regs.STAT.val) ) {
        // The active flags were all low, so the new flag could trigger an interrupt if it is also enabled!
        activeStatFlags |= newFlag;
        if( 0 != (activeStatFlags & regs.STAT.val) ) {
            setIntFlag(INT_STAT);
        }
    } else {
        // there's already an acitve interrupt, so no interrupt will be triggered,
        // but we still need to set the new flag
        activeStatFlags |= newFlag;
    }
}

void checkLYC(void)
{
    // LYC is continually tested against LY so we need to re-check any time either changes
    if( regs.LYC.val == regs.LY.val ) {
        regs.STAT.lycMatch = 1;
        if( 1 == regs.STAT.lycIntEnable ) {
            maybeTriggerStatInterrupt(INT_STAT_LYC);
        }
    } else {
        regs.STAT.lycMatch = 0;
        activeStatFlags &= ~INT_STAT_LYC;
    }
}

void setGfxReg8(uint16_t addr, uint8_t val8)
{
    switch( addr ) {
        case REG_LCDC_ADDR:
            regs.LCDC.val = val8;
            if(0 == regs.LCDC.graphicsEnable) {
                // reset the PPU state?
            }
            return;
        case REG_STAT_ADDR:
            // check if this update would cause a low-to-high transition in selected interrupts
            if( (0 != (activeStatFlags & val8)) && (0 == (activeStatFlags & regs.STAT.val)) ) {
                setIntFlag(INT_STAT);
            }
            regs.STAT.val = val8;
            checkLYC();  // in case interrupt enable flag was changed
            return;
        case REG_SCY_ADDR:
            regs.SCY.val = val8;
            return;
        case REG_SCX_ADDR:
            regs.SCX.val = val8;
            return;
        case REG_LY_ADDR:
            regs.LY.val = val8;
            checkLYC();
            return;
        case REG_LYC_ADDR:
            regs.LYC.val = val8;
            checkLYC();
            return;
        case REG_BGP_ADDR:
            regs.BGP.val = val8;
            return;
        case REG_OBP0_ADDR:
            regs.OBP0.val = val8;
            return;
        case REG_OBP1_ADDR:
            regs.OBP1.val = val8;
            return;
        case REG_WY_ADDR:
            regs.WY.val = val8;
            return;
        case REG_WX_ADDR:
            regs.WY.val = val8;
            return;
    }
}

uint8_t getGfxReg8(uint16_t addr)
{
    switch( addr ) {
        case REG_LCDC_ADDR:
            return regs.LCDC.val;
        case REG_STAT_ADDR:
            return regs.STAT.val;
        case REG_SCY_ADDR:
            return regs.SCY.val;
        case REG_SCX_ADDR:
            return regs.SCX.val;
        case REG_LY_ADDR:
            if( NULL != doctorLogFile ) {
                // special case when running tests
                return 0x90;
            } else {
                return regs.LY.val;
            }
        case REG_LYC_ADDR:
            return regs.LYC.val;
        case REG_BGP_ADDR:
            return regs.BGP.val;
        case REG_OBP0_ADDR:
            return regs.OBP0.val;
        case REG_OBP1_ADDR:
            return regs.OBP1.val;
        case REG_WY_ADDR:
            return regs.WY.val;
        case REG_WX_ADDR:
            return regs.WX.val;
        default:
            return 0x00;
    }
}


#define MODE_OAM    (2)
#define MODE_DRAW   (3)
#define MODE_HBLANK (0)
#define MODE_VBLANK (1)

#define OAM_CYCLES          (80)    // OAM Scan
#define DRAW_MIN_CYCLES     (172)   // Drawing
#define DRAW_MAX_CYCLES     (289)
#define HBLANK_MIN_CYCLES   (87)    // HBLANK
#define HBLANK_MAX_CYCLES   (204)
#define SCANLINE_CYCLES     (OAM_CYCLES + DRAW_MIN_CYCLES + HBLANK_MAX_CYCLES)  // 456 cycles
#define VBLANK_CYCLES       (SCANLINE_CYCLES * 10)  // VBLANK 4560 cycles
#define FRAME_CYCLES        (SCANLINE_CYCLES * 144 + VBLANK_CYCLES)  // 70224 cycles

#define LCD_DISPLAY_LINES   (144)
#define LCD_VBLANK_LINES    (10)
#define LCD_TOTAL_LINES     (LCD_DISPLAY_LINES + LCD_VBLANK_LINES)

static int frameCounter = 0;
static int scanlineCounter = 0;



void ppuCycles(int cycles)
{
    while(cycles--) {
        if(1 == regs.LCDC.graphicsEnable) {
            frameCounter++;
            scanlineCounter++;

            switch( regs.STAT.ppuMode ) {
                case MODE_OAM:
                    if( OAM_CYCLES <= scanlineCounter ) {
                        // advance to DRAW
                        regs.STAT.ppuMode = MODE_DRAW;
                        activeStatFlags &= ~INT_STAT_OAM;
                    }
                    break;

                case MODE_DRAW:

                    if( (OAM_CYCLES + DRAW_MIN_CYCLES) <= scanlineCounter ) {
                        // advance to HBLANK
                        regs.STAT.ppuMode = MODE_HBLANK;
                        maybeTriggerStatInterrupt(INT_STAT_HBLANK);
                    }
                    break;

                case MODE_HBLANK:

                    if( (SCANLINE_CYCLES) <= scanlineCounter ) {
                        scanlineCounter = 0;
                        regs.LY.val++;

                        checkLYC();

                        if( LCD_DISPLAY_LINES < regs.LY.val ) {
                            regs.STAT.ppuMode = MODE_VBLANK;
                            maybeTriggerStatInterrupt(INT_STAT_VBLANK);
                            setIntFlag(INT_VBLANK);  // always triggered
                        } else {
                            regs.STAT.ppuMode = MODE_OAM;
                            maybeTriggerStatInterrupt(INT_STAT_OAM);
                        }
                        activeStatFlags &= ~INT_STAT_HBLANK;
                    }
                    break;

                case MODE_VBLANK:

                    if( (SCANLINE_CYCLES) <= scanlineCounter ) {
                        scanlineCounter = 0;
                        regs.LY.val++;
                        if( LCD_TOTAL_LINES <= regs.LY.val ) {
                            regs.LY.val = 0;
                            frameCounter = 0;

                            regs.STAT.ppuMode = MODE_OAM;
                            maybeTriggerStatInterrupt(INT_STAT_OAM);
                            activeStatFlags &= ~INT_STAT_VBLANK;
                        }

                        checkLYC();

                    }
                    break;
            }
        }
    }
}

RamImage vram;

void graphicsInit(void)
{
    memset(&regs, 0, sizeof(regs));
    frameCounter = 0;
    scanlineCounter = 0;
    activeStatFlags = 0;
    allocateRam(&vram, 8192);
    addRamView(&vram, "VRAM", 0x8000);
}

static const Color paletteColor[4] = {
    WHITE,
    LIGHTGRAY,
    DARKGRAY,
    BLACK
};

typedef struct {
    struct {
        uint8_t lBits;
        uint8_t hBits;
    } line[8];
} Tile;

static void guiDrawTile(Vector2 anchor, Tile *tile, PaletteReg pal, float pixelSize, float pixelPad)
{
    Rectangle pixelRect = { anchor.x, anchor.y, pixelSize, pixelSize };
    for( int y = 0; y < 8; y++ ) {
        pixelRect.x = anchor.x;
        for( int x = 0; x < 8; x++ ) {
            int pixelPalIdx = (BIT(tile->line[y].hBits, 7-x) << 1) | BIT(tile->line[y].lBits, 7-x);
            int palColor = (pal.val & (3<<(pixelPalIdx*2))) >> (pixelPalIdx*2);
            DrawRectangleRec(pixelRect, paletteColor[palColor]);
            pixelRect.x += pixelSize+pixelPad;
        }
        pixelRect.y += pixelSize+pixelPad;
    }
}

static void guiDrawMapFrame(Vector2 anchor, uint16_t x, uint16_t y, Color color)
{
    float left, right, top, bottom;
    //bottom := (SCY + 143) % 256 and right := (SCX + 159) % 256
    left = anchor.x + x;
    right = anchor.x + ((x + 159) % 256);
    top = anchor.y + y;
    bottom = ((y + 143) % 256) + anchor.y;
    if(left < right) {
        // normal top & bottom lines
        DrawLine(left, top, right, top, color);
        DrawLine(left, bottom, right, bottom, color);
        // extra thickk
        DrawLine(left, top+0.5, right, top+0.5, color);
        DrawLine(left, bottom-0.5, right, bottom-0.5, color);
    } else {
        // split top & bottom lines
        DrawLine(anchor.x, top, right, top, color);
        DrawLine(left, top, anchor.x+8*32, top, color);

        DrawLine(anchor.x, bottom, right, bottom, color);
        DrawLine(left, bottom, anchor.x+8*32, bottom, color);
        // extra thickk
        DrawLine(anchor.x, top+0.5, right, top+0.5, color);
        DrawLine(left, top+0.5, anchor.x+8*32, top+0.5, color);

        DrawLine(anchor.x, bottom-0.5, right, bottom-0.5, color);
        DrawLine(left, bottom-0.5, anchor.x+8*32, bottom-0.5, color);
    }
    if(top < bottom) {
        // normal left & right lines
        DrawLine(left, top, left, bottom, color);
        DrawLine(right, top, right, bottom, color);
        // extra thickk
        DrawLine(left+0.5, top, left+0.5, bottom, color);
        DrawLine(right-0.5, top, right-0.5, bottom, color);
    } else {
        // split left & right lines
        DrawLine(left, anchor.y, left, bottom, color);
        DrawLine(left, top, left, anchor.y+8*32, color);

        DrawLine(right, anchor.y, right, bottom, color);
        DrawLine(right, top, right, anchor.y+8*32, color);
        // extra thickk
        DrawLine(left+0.5, anchor.y, left+0.5, bottom, color);
        DrawLine(left+0.5, top, left+0.5, anchor.y+8*32, color);

        DrawLine(right-0.5, anchor.y, right-0.5, bottom, color);
        DrawLine(right-0.5, top, right-0.5, anchor.y+8*32, color);
    }
}

static void guiDrawTileMap(Vector2 anchor, const uint8_t map)
{
    // Width x Height = 256 x 256 or 288 x 288
    Vector2 tileAnchor = anchor;
    uint8_t tileIndex;
    Tile *tile;
    uint16_t offset = 0x1800 + 0x400 * map;

    for( int y = 0; y < 32; y++ ) {
        tileAnchor.x = anchor.x;
        for( int x = 0; x < 32; x++ ) {
            tileIndex = vram.contents[offset++];
            if( 1 == regs.LCDC.bgWinTileData ) {
                tile = (Tile *)&vram.contents[tileIndex*16];
            } else {
                tile = (Tile *)&vram.contents[ (256+((int8_t)tileIndex))*16 ];
            }
            guiDrawTile(tileAnchor, tile, regs.BGP, 1, 0);
            tileAnchor.x += 8;
        }
        tileAnchor.y += 8;
    }

    if( 1 == regs.LCDC.bgWinEnable ) {
        if( map == regs.LCDC.bgTileMap ) {
            // draw background frame
            guiDrawMapFrame(anchor, regs.SCX.val, regs.SCY.val, PURPLE);
        }
        if( (1 == regs.LCDC.windowEnable)
         && (map == regs.LCDC.windowTileMap) ) {
             // draw window frame
             guiDrawMapFrame(anchor, 166-regs.WX.val, 143-regs.WY.val, BLUE);
        }
    }
}

static void guiDrawTileData(Vector2 anchor)
{
    // Width x Height = 18*17 x 25*17 = 306 x 425

    uint16_t offset = 0;
    Vector2 tileAnchor = anchor;

    const Color lineColor = GetColor(GuiGetStyle(DEFAULT,LINE_COLOR));

    tileAnchor.x += 4 + FONTWIDTH*2;
    for( int x = 0; x < 16; x++ ) {
        DrawTextEx(firaFont, TextFormat("%X",x), tileAnchor, FONTSIZE, 0, lineColor);
        tileAnchor.x += 2*8+1;
    }
    tileAnchor = (Vector2){ anchor.x, anchor.y + FONTSIZE };
    for( int y = 0; y < 24; y++ ) {
        tileAnchor.x = anchor.x;

        // Draw indexes for selected BG/Win tile data
        if( y < 8 ) {
            if (1 == regs.LCDC.bgWinTileData) {
                DrawTextEx(firaFont, TextFormat("%X0",y), tileAnchor, FONTSIZE, 0, lineColor);
            }
        } else if( y < 16 ) {
            DrawTextEx(firaFont, TextFormat("%X0",y), tileAnchor, FONTSIZE, 0, lineColor);
        } else if( 0 == regs.LCDC.bgWinTileData ) {
            DrawTextEx(firaFont, TextFormat("%X0",y-16), tileAnchor, FONTSIZE, 0, lineColor);
        }
        tileAnchor.x += FONTWIDTH*2;

        for( int x = 0; x < 16; x++ ) {
            guiDrawTile(tileAnchor, (Tile *)&vram.contents[offset], regs.BGP, 2, 0);
            offset += sizeof(Tile);
            tileAnchor.x += 2*8+1;
        }

        // Draw indexes for Obj tiles
        if( y < 16 ) {
            DrawTextEx(firaFont, TextFormat("%X0",y), tileAnchor, FONTSIZE, 0, lineColor);
        }
        tileAnchor.y += 2*8+1;
    }
}
/*
static void guiDrawCpuReg8(Vector2 anchor, uint8_t val8, const char * const name)
{
    // header
    GuiGroupBox((Rectangle){anchor.x, anchor.y, FONTWIDTH*4, FONTSIZE*1.5}, name);
    //DrawTextEx(firaFont, name, anchor, FONTSIZE, 0, BLACK);
    anchor.x += FONTWIDTH;
    anchor.y += FONTSIZE/3;
    DrawTextEx(firaFont, TextFormat("%02X", val8),  anchor, FONTSIZE, 0, BLACK);
}
*/

void guiDrawGraphics(void)
{
    // Top left corner of the memory viewer
    Vector2 viewAnchor = { 510, 100 };
    guiDrawTileMap((Vector2){550, 50}, 0);
    guiDrawTileMap((Vector2){550, 350}, 1);
    guiDrawTileData((Vector2){850, 50});
}

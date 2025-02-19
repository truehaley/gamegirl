#include "cpu.h"
#include "gb.h"

struct {
    struct {
        uint8_t val;
    } WX;  // FF4B

    struct {
        uint8_t val;
    } WY;   // FF4A

    struct {
        union {
            uint8_t val;
            struct {
                uint8_t transparent:2;
                uint8_t palCol1:2;
                uint8_t palCol2:2;
                uint8_t palCol3:2;
            };
        };
    } OBP1;  // FF49

    struct {
        union {
            uint8_t val;
            struct {
                uint8_t transparent:2;
                uint8_t palCol1:2;
                uint8_t palCol2:2;
                uint8_t palCol3:2;
            };
        };
    } OBP0;  // FF48

    struct {
        union {
            uint8_t val;
            struct {
                uint8_t palCol0:2;
                uint8_t palCol1:2;
                uint8_t palCol2:2;
                uint8_t palCol3:2;
            };
        };
    } BGP;  // FF47

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
                uint8_t bgWindowEnable:1;   // 0 = Off; 1 = On
                uint8_t objEnable:1;        // 0 = Off; 1 = On
                uint8_t objSize:1;          // 0 = 8×8; 1 = 8×16
                uint8_t bgTileMap:1;        // 0 = 9800–9BFF; 1 = 9C00–9FFF
                uint8_t bgWinTileData:1;    // 0 = 8800–97FF (signed); 1 = 8000–8FFF (unsigned)
                uint8_t windowEnable:1;     // 0 = Off; 1 = On
                uint8_t windowTileMap:1;    // 0 = 9800–9BFF; 1 = 9C00–9FFF
                uint8_t graphicsEnable:1;   // 0 = Off; 1 = On
            };
        };
    } LCDL;  // FF40
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
        case REG_LCDL_ADDR:
            regs.LCDL.val = val8;
            if(0 == regs.LCDL.graphicsEnable) {
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
        case REG_LCDL_ADDR:
            return regs.LCDL.val;
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
        if(1 == regs.LCDL.graphicsEnable) {
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


void graphicsInit(void)
{
    memset(&regs, 0, sizeof(regs));
    frameCounter = 0;
    scanlineCounter = 0;
    activeStatFlags = 0;
}

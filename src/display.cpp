#include "gb.h"
#include "gui.h"

// Set at the beginning of vblank so the gui will redraw the screen
bool guiUpdateScreen = false;

typedef struct {
    struct {
        uint8_t lBits;
        uint8_t hBits;
    } line[8];
} Tile;

typedef struct {
    Image image;
    Texture2D tex;
    bool dirty;
} TileTex;

struct {
    union{
        uint8_t contents[0x2000];
        struct {
            Tile tiles[384];
            struct {
                uint8_t tileRef[32][32];
            } tileMap[2];
        };
    };
} vram;

TileTex tileTextures[384];

static RamImage vramImage;


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

#define PALETTE_COLOR(paletteReg, palIdx) (((paletteReg) & (0x3<<((palIdx)*2))) >> (palIdx*2))

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
    } OAM;  // FF46

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
                uint8_t displayEnable:1;   // 0 = Off; 1 = On
            };
        };
    } LCDC;  // FF40
} regs;

int screenData[SCREEN_HEIGHT][SCREEN_WIDTH];

#define INT_STAT_HBLANK (0x08)
#define INT_STAT_VBLANK (0x10)
#define INT_STAT_OAM    (0x20)
#define INT_STAT_LYC    (0x40)
#define INT_STAT_ENABLES_MASK  (INT_STAT_HBLANK | INT_STAT_VBLANK | INT_STAT_OAM | INT_STAT_LYC)
uint8_t activeStatFlags = 0;

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

#define LCD_VBLANK_LINES    (10)
#define LCD_TOTAL_SCANLINES (SCREEN_HEIGHT + LCD_VBLANK_LINES)



static int frameCounter = 0;
static int scanlineCounter = 0;
static int totalFrames = 0;


void setVram8(uint16_t addr, uint8_t val8)
{
    vram.contents[addr&0x1FFF] = val8;
    if( sizeof(vram.tiles) > addr ) {
        tileTextures[addr/sizeof(Tile)].dirty = true;
    }
}

uint8_t getVram8(uint16_t addr)
{
    if( MODE_DRAW == regs.STAT.ppuMode ) {
        return 0xFF;
    }
    return vram.contents[addr&0x1FFF];
}

typedef struct __attribute__((packed)) {
    uint8_t yPos;
    uint8_t xPos;
    uint8_t tileIndex;
    union {
        uint8_t val;
        struct {
            uint8_t reserved:4;
            uint8_t palette:1;
            uint8_t xFlip:1;
            uint8_t yFlip:1;
            uint8_t priority:1;
        };
    } attributes;
} OamEntry;

#define OAM_ENTRIES     (40)
#define OAM_SIZE        (OAM_ENTRIES * sizeof(OamEntry))

union {
    uint8_t contents[OAM_SIZE];
    OamEntry entries[OAM_ENTRIES];
} oamRam;

static RamImage oamImage;


static uint8_t oamDmaOffset = OAM_SIZE;
static enum { IDLE, DELAY, START } oamDmaStart = IDLE;

void setOam8(uint16_t addr, uint8_t val8)
{
    assert(addr < OAM_SIZE);
    oamRam.contents[addr] = val8;
}

uint8_t getOam8(uint16_t addr)
{
    assert(addr < OAM_SIZE);
    if( MODE_DRAW == regs.STAT.ppuMode || MODE_OAM == regs.STAT.ppuMode ) {
        return 0xFF;
    }
    return oamRam.contents[addr];
}

void oamDmaCycle(void)
{
    if( OAM_SIZE > oamDmaOffset ) {
        oamRam.contents[oamDmaOffset] = getRawMem8((regs.OAM.val << 8) + oamDmaOffset);
        oamDmaOffset++;
    }
    if( START == oamDmaStart ) {
        oamDmaStart = DELAY;
    } else if( DELAY == oamDmaStart ) {
        oamDmaStart = IDLE;
        oamDmaOffset = 0;
    }
}

bool oamDmaActive(void)
{
    return ( OAM_SIZE > oamDmaOffset );
}

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

void setGfxReg8(uint16_t addr, const uint8_t val8)
{
    switch( addr ) {
        case REG_LCDC_ADDR:
            regs.LCDC.val = val8;
            if(0 == regs.LCDC.displayEnable) {
                // reset the PPU state
                // initialize framecounter to a value that accounts for where we were in the frame
                //  when the lcd was turned off.
                frameCounter = SCANLINE_CYCLES*regs.LY.val + scanlineCounter;
                regs.LY.val = 0;
                regs.STAT.ppuMode = 0;
                scanlineCounter=0;
                activeStatFlags=0;
            } else {
                //regs.STAT.ppuMode = MODE_OAM;
            }
            return;
        case REG_STAT_ADDR:
            // check if this update would cause a low-to-high transition in selected interrupts
            if( (0 != (activeStatFlags & val8)) && (0 == (activeStatFlags & regs.STAT.val)) ) {
                setIntFlag(INT_STAT);
            }
            // only the enable flags are writeable
            regs.STAT.val = (regs.STAT.val & ~INT_STAT_ENABLES_MASK) | (val8 & INT_STAT_ENABLES_MASK);
            checkLYC();  // in case interrupt enable flag was changed
            return;
        case REG_SCY_ADDR:
            regs.SCY.val = val8;
            return;
        case REG_SCX_ADDR:
            regs.SCX.val = val8;
            return;
        case REG_LY_ADDR:
            // Not writeable
            //regs.LY.val = val8;
            //checkLYC();
            return;
        case REG_LYC_ADDR:
            regs.LYC.val = val8;
            checkLYC();
            return;
        case REG_OAM_ADDR:
            regs.OAM.val = val8;
            // kickoff the dma on the next clock cycle
            oamDmaStart = START;
            return;
        case REG_BGP_ADDR:
            if( val8 != regs.BGP.val ) {
                // Palette update, mark all tiles as dirty
                for(int i=0; i<384; i++) {
                    tileTextures[i].dirty = true;
                }
            }
            regs.BGP.val = val8;
            return;
        case REG_OBP0_ADDR:
            if( val8 != regs.BGP.val ) {
                // Palette update, mark all tiles as dirty
                for(int i=0; i<384; i++) {
                    tileTextures[i].dirty = true;
                }
            }
            regs.OBP0.val = val8;
            return;
        case REG_OBP1_ADDR:
            if( val8 != regs.BGP.val ) {
                // Palette update, mark all tiles as dirty
                for(int i=0; i<384; i++) {
                    tileTextures[i].dirty = true;
                }
            }
            regs.OBP1.val = val8;
            return;
        case REG_WY_ADDR:
            regs.WY.val = val8;
            return;
        case REG_WX_ADDR:
            regs.WX.val = val8;
            return;
    }
}

uint8_t getGfxReg8(uint16_t addr)
{
    switch( addr ) {
        case REG_LCDC_ADDR:
            return regs.LCDC.val;
        case REG_STAT_ADDR:
            regs.STAT.reserved = 0x1;   // reads as 1s
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
        case REG_OAM_ADDR:
            return regs.OAM.val;
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


class BgFetcher {
protected:
    enum {
        TILEREF_1 = 0,
        TILEREF_2,
        TILEDATA_LOW_1,
        TILEDATA_LOW_2,
        TILEDATA_HIGH_1,
        TILEDATA_HIGH_2,
        FIFO_PUSH_1,
        FIFO_PUSH_2
    } state;
    bool newline;
    int xTile;
    //int windowLine;

    struct {
        uint8_t hBits;  // 8 entries, 2 bits per pixel
        uint8_t lBits;
        int depth;
    } fifo;

    struct {
        int map;
        int x,y;
        int row;
        int ref;
        Tile *tile;
        uint8_t lBits, hBits;
    } tileInfo;

public:
    void reset(bool newScanline, bool windowMode) {
        state = TILEREF_1;
        newline = newScanline;
        if( newScanline || windowMode ) {
            xTile = 0;
        }
        //windowLine = 0;
        fifo.depth = 0;
    }

    bool empty(void) {
        return (0 == fifo.depth);
    }

    uint8_t pop(void) {
        assert( !empty() );
        int palIdx = ((fifo.hBits & 0x80) >> 6) | (((fifo.lBits & 0x80) >> 7));
        fifo.hBits <<= 1;
        fifo.lBits <<= 1;
        fifo.depth--;
        return palIdx;
    }

    void cycle(uint8_t xCoord, bool windowMode, uint8_t windowLine) {
        switch(state) {
            case TILEREF_1:
                if( windowMode ) {
                    tileInfo.map = regs.LCDC.windowTileMap;
                    tileInfo.x = (xTile & 0x1F);
                    tileInfo.y = (windowLine / 8) & 0x1F;
                    tileInfo.row = (windowLine & 0x7);
                } else {
                    tileInfo.map = regs.LCDC.bgTileMap;
                    tileInfo.x = ((regs.SCX.val/8) + xTile) & 0x1F;
                    tileInfo.y = ((regs.SCY.val + regs.LY.val)/8) & 0x1F;
                    tileInfo.row = ((regs.SCY.val + regs.LY.val) & 0x7);
                }
                state = TILEREF_2;
                break;
            case TILEREF_2:
                assert(tileInfo.map < 2);
                assert(tileInfo.x < 32);
                assert(tileInfo.y < 32);

                if( 1 == regs.LCDC.bgWinTileData ) {
                    tileInfo.ref = vram.tileMap[tileInfo.map].tileRef[tileInfo.y][tileInfo.x];
                    assert( (0 <= tileInfo.ref) && (256 > tileInfo.ref) );
                } else {
                    tileInfo.ref = 256 + (int8_t)vram.tileMap[tileInfo.map].tileRef[tileInfo.y][tileInfo.x];
                    assert( (128 <= tileInfo.ref ) && (384 > tileInfo.ref) );
                }
                tileInfo.tile = &vram.tiles[tileInfo.ref];
                state = TILEDATA_LOW_1;
                break;

            case TILEDATA_LOW_1:
                // just a timing delay
                state = TILEDATA_LOW_2;
                break;
            case TILEDATA_LOW_2:
                assert(tileInfo.row < 8);
                tileInfo.lBits = tileInfo.tile->line[tileInfo.row].lBits;
                state = TILEDATA_HIGH_1;
                break;

            case TILEDATA_HIGH_1:
                // just a timing delay
                state = TILEDATA_HIGH_2;
                break;
            case TILEDATA_HIGH_2:
                tileInfo.hBits = tileInfo.tile->line[tileInfo.row].hBits;
                // The first time the background fetcher completes this step on a scanline the status is
                //  fully reset and operation restarts at Step 1.
                if( true == newline ) {
                    state = TILEREF_1;
                    newline = false;
                } else {
                    state = FIFO_PUSH_1;
                }
                break;

            case FIFO_PUSH_1:
                if( empty() ) {
                    // add pixels to the fifo!
                    fifo.lBits = tileInfo.lBits;
                    fifo.hBits = tileInfo.hBits;
                    fifo.depth = 8;
                    xTile++;
                    state = FIFO_PUSH_2;
                } else {
                    // we stay here until the fifo is empty
                    state = FIFO_PUSH_1;
                }
                break;
            case FIFO_PUSH_2:
                // just a timing delay
                state = TILEREF_1;
                break;
        }
    }
} bgFetch;

typedef struct {
    uint8_t palRef:2;
    uint8_t pal:1;
    uint8_t pri:1;
} ObjPixel;

class ObjFetcher {
protected:
    enum {
        TILEREF_1 = 0,
        TILEREF_2,
        TILEDATA_LOW_1,
        TILEDATA_LOW_2,
        TILEDATA_HIGH_1,
        TILEDATA_HIGH_2,
        FIFO_PUSH_1,
        FIFO_PUSH_2
    } state;

    struct {
        uint8_t hBits;  // 8 entries, 2 bits per pixel
        uint8_t lBits;
        uint8_t pri;
        uint8_t pal;
        int depth;
    } fifo;

    struct {
        int row;
        int ref;
        int visible;
        Tile *tile;
        uint8_t lBits, hBits;
    } tileInfo;

public:
    void reset(bool fullReset) {
        state = TILEREF_1;
        if(true == fullReset) {
            fifo.lBits = 0;
            fifo.hBits = 0;
            fifo.depth = 0;
        }
    }

    bool empty(void) {
        return (0 == fifo.depth);
    }

    ObjPixel pop(void) {
        assert( !empty() );
        ObjPixel pix;
        pix.palRef = ((fifo.hBits & 0x80) >> 6) | (((fifo.lBits & 0x80) >> 7));
        pix.pal = ((fifo.pal & 0x80) >> 7);
        pix.pri = ((fifo.pri & 0x80) >> 7);
        fifo.hBits <<= 1;
        fifo.lBits <<= 1;
        fifo.pri <<=1;
        fifo.pal <<=1;
        fifo.depth--;
        return pix;
    }

    bool cycle(OamEntry *object) {
        switch(state) {
            case TILEREF_1:
                tileInfo.ref = (0 == regs.LCDC.objSize)? object->tileIndex : (object->tileIndex & 0xFE);
                tileInfo.visible = MIN(8 , object->xPos);
                tileInfo.row = ((regs.LY.val - object->yPos) & 0xF);
                if( 1 == object->attributes.yFlip ) {
                    tileInfo.row = (0 == regs.LCDC.objSize)? (7 - tileInfo.row) : (15 - tileInfo.row);
                }
                state = TILEREF_2;
                break;
            case TILEREF_2:
                tileInfo.tile = &vram.tiles[tileInfo.ref];
                state = TILEDATA_LOW_1;
                break;

            case TILEDATA_LOW_1:
                // just a timing delay
                state = TILEDATA_LOW_2;
                break;
            case TILEDATA_LOW_2:
                assert((0 == regs.LCDC.objSize)? (tileInfo.row < 8) : (tileInfo.row < 16));
                tileInfo.lBits = tileInfo.tile->line[tileInfo.row].lBits;
                if( 1 == object->attributes.xFlip ) {
                    tileInfo.lBits = ((tileInfo.lBits & 0xaa) >> 1) | ((tileInfo.lBits & 0x55) << 1);
                    tileInfo.lBits = ((tileInfo.lBits & 0xcc) >> 2) | ((tileInfo.lBits & 0x33) << 2);
                    tileInfo.lBits = ((tileInfo.lBits & 0xf0) >> 4) | ((tileInfo.lBits & 0x0f) << 4);
                }
                state = TILEDATA_HIGH_1;
                break;

            case TILEDATA_HIGH_1:
                // just a timing delay
                state = TILEDATA_HIGH_2;
                break;
            case TILEDATA_HIGH_2:
                tileInfo.hBits = tileInfo.tile->line[tileInfo.row].hBits;
                if( 1 == object->attributes.xFlip ) {
                    tileInfo.hBits = ((tileInfo.hBits & 0xaa) >> 1) | ((tileInfo.hBits & 0x55) << 1);
                    tileInfo.hBits = ((tileInfo.hBits & 0xcc) >> 2) | ((tileInfo.hBits & 0x33) << 2);
                    tileInfo.hBits = ((tileInfo.hBits & 0xf0) >> 4) | ((tileInfo.hBits & 0x0f) << 4);
                }
                state = FIFO_PUSH_1;
                break;

            case FIFO_PUSH_1: {
                // new pixels need to be mixed in with existing pixels
                uint8_t emptyBits = ~(fifo.hBits | fifo.lBits);
                fifo.lBits |= ((tileInfo.lBits << (8-tileInfo.visible)) & emptyBits);
                fifo.hBits |= ((tileInfo.hBits << (8-tileInfo.visible)) & emptyBits);
                fifo.pal = (fifo.pal & ~emptyBits) | (((0 == object->attributes.palette)? 0x00 : 0xFF) & emptyBits);
                fifo.pri = (fifo.pri & ~emptyBits) | (((0 == object->attributes.priority)? 0x00 : 0xFF) & emptyBits);
                fifo.depth = tileInfo.visible;
                state = FIFO_PUSH_2;
                break;
            }
            case FIFO_PUSH_2:
                // just a timing delay
                state = TILEREF_1;
                return true;
                break;
        }
        return false;
    }
} objFetch;

#define MAX_OBJECTS_PER_LINE    (10)
OamEntry scanlineObjects[MAX_OBJECTS_PER_LINE];

void ppuCycles(int cycles)
{
    static uint8_t xSkip = 0;
    static uint8_t xCoordinate = 0;
    static bool windowActive = false;
    static uint8_t windowLine = 0;
    static int foundObjects = 0;
    static OamEntry *objInProcess = nullptr;

    while(cycles--) {
        if(0 == regs.LCDC.displayEnable) {
            // When the LCD is disabled, we still make use a frame counter to make sure that the
            //  emulator UI is refreshed at roughly the same 60Hz rate.
            // frameCounter is intialized when the LCD is disabled to account for any time already
            //   spent in the current refresh cycle.
            frameCounter++;
            if(FRAME_CYCLES <= frameCounter) {
                guiUpdateScreen = true;
                frameCounter = 0;
            }

        } else { // if(1 == regs.LCDC.displayEnable) {
            scanlineCounter++;

            switch( regs.STAT.ppuMode ) {
                case MODE_OAM:
                    // process one OAM entry every other cycle
                    if( 0x01 == (scanlineCounter & 0x01)) {
                        OamEntry *object = &oamRam.entries[scanlineCounter >> 1];

                        /*  oam.x != 0
                            LY+ 16 >= oam.y
                            LY+ 16 < oam.y+h    */
                        if( ((regs.LY.val + 16) >= object->yPos)
                        &&  ((regs.LY.val + 16) < (object->yPos + ((0 == regs.LCDC.objSize)? 8 : 16)))
                        &&  (MAX_OBJECTS_PER_LINE > foundObjects) ) {
                            scanlineObjects[foundObjects++] = *object;
                        }
                    }

                    if( OAM_CYCLES <= scanlineCounter ) {
                        // advance to DRAW
                        xCoordinate = 0;
                        windowActive = false;
                        regs.STAT.ppuMode = MODE_DRAW;
                        activeStatFlags &= ~INT_STAT_OAM;
                        bgFetch.reset(true, false);
                        xSkip = regs.SCX.val & 0x7;
                        objInProcess = nullptr;
                    }
                    break;

                case MODE_DRAW:

                    if( nullptr == objInProcess ) {
                        // check
                        for(int i=0; i<foundObjects; i++) {
                            if( xCoordinate + 8 >= scanlineObjects[i].xPos ) {
                                objInProcess = &scanlineObjects[i];
                                //bgFetch.reset(false, false);
                                objFetch.reset(false);
                                objFetch.cycle(objInProcess);
                                break;
                            }
                        }
                    } else {
                        // object fetching takes precedence
                        if( true == objFetch.cycle(objInProcess) ) {
                            // this object fetch is complete
                            // set the x val really high so it isn't processed again
                            objInProcess->xPos = 0xFF;
                            objInProcess = nullptr;
                        }
                        break;
                    }
                    if(nullptr != objInProcess) {
                        break;
                    }


                    bgFetch.cycle(xCoordinate, windowActive, windowLine);
                    if(!bgFetch.empty()) {
                        int bgPalRef = bgFetch.pop();

                        if(0 == xCoordinate && 0 < xSkip) {
                            xSkip--;
                        } else {
                            assert(regs.LY.val < SCREEN_HEIGHT);
                            assert(xCoordinate < SCREEN_WIDTH);

                            if(!objFetch.empty()) {
                                ObjPixel objPix = objFetch.pop();
                                uint8_t palette = (0==objPix.pal)? regs.OBP0.val : regs.OBP1.val;
                                if(0 == objPix.pri) {
                                    // object takes priority
                                    if( (1 == regs.LCDC.objEnable) && (0 != objPix.palRef) ) {
                                        screenData[regs.LY.val][xCoordinate] = PALETTE_COLOR(palette, objPix.palRef);
                                    } else if( 1 == regs.LCDC.bgWinEnable ) {
                                        screenData[regs.LY.val][xCoordinate] = PALETTE_COLOR(regs.BGP.val, bgPalRef);
                                    } else {
                                        screenData[regs.LY.val][xCoordinate] = 0;
                                    }
                                } else {
                                    // background takes priority
                                    if( (1 == regs.LCDC.objEnable) && (0 == bgPalRef) && (0 != objPix.palRef) ) {
                                        screenData[regs.LY.val][xCoordinate] = PALETTE_COLOR(palette, objPix.palRef);
                                    } else if( 1 == regs.LCDC.bgWinEnable ) {
                                        screenData[regs.LY.val][xCoordinate] = PALETTE_COLOR(regs.BGP.val, bgPalRef);
                                    } else {
                                        screenData[regs.LY.val][xCoordinate] = 0;
                                    }
                                }

                            } else {
                                if(regs.LCDC.bgWinEnable) {
                                    screenData[regs.LY.val][xCoordinate] = PALETTE_COLOR(regs.BGP.val, bgPalRef);
                                } else {
                                    screenData[regs.LY.val][xCoordinate] = 0;
                                }
                            }
                            xCoordinate++;

                            if(true == regs.LCDC.windowEnable) {
                                if( (0 < windowLine) || (regs.WY.val == regs.LY.val) ) {
                                    if( !windowActive && (regs.WX.val - 7) <= xCoordinate ) {
                                        windowActive = true;
                                        bgFetch.reset(false, true);
                                    }
                                }
                            }
                        }
                    }

                    if( SCREEN_WIDTH <= xCoordinate  ) {
                        assert((OAM_CYCLES + DRAW_MAX_CYCLES) >= scanlineCounter);
                        // advance to HBLANK
                        regs.STAT.ppuMode = MODE_HBLANK;
                        maybeTriggerStatInterrupt(INT_STAT_HBLANK);
                        objFetch.reset(true);
                    }
                    break;

                case MODE_HBLANK:
                    if( (SCANLINE_CYCLES) <= scanlineCounter ) {
                        scanlineCounter = 0;
                        regs.LY.val++;

                        checkLYC();

                        if( SCREEN_HEIGHT <= regs.LY.val ) {
                            regs.STAT.ppuMode = MODE_VBLANK;
                            maybeTriggerStatInterrupt(INT_STAT_VBLANK);
                            setIntFlag(INT_VBLANK);  // always triggered
                            windowLine = 0;
                            if(true == bootRomActive) {
                                if( false == fastBoot ) {
                                    // Even if we're not in fastboot mode, we refresh the gui 10 times less
                                    //  often while the bootrom is running, letting us speed through the boot screen!
                                    guiUpdateScreen = ((totalFrames % 60) == 0);
                                }
                            } else {
                                guiUpdateScreen = true;
                            }
                        } else {
                            regs.STAT.ppuMode = MODE_OAM;
                            maybeTriggerStatInterrupt(INT_STAT_OAM);
                            if(true == windowActive) {
                                windowLine++;
                            }
                            memset(scanlineObjects, 0xFF, sizeof(scanlineObjects));
                            foundObjects = 0;
                        }
                        activeStatFlags &= ~INT_STAT_HBLANK;
                    }
                    break;

                case MODE_VBLANK:
                    if( (153 == regs.LY.val) && (4 < scanlineCounter) ) {
                        // scanline 153 quirk... LY gets set to 0 after only 4 cycles
                        regs.LY.val = 0;
                        checkLYC();
                    }
                    if( (SCANLINE_CYCLES) <= scanlineCounter ) {
                        totalFrames++;
                        scanlineCounter = 0;
                        // will have been set to 0 above as part of scanline 153 quirk handling
                        if( 0 == regs.LY.val ) {
                            regs.LY.val = 0;
                            frameCounter = 0;

                            regs.STAT.ppuMode = MODE_OAM;
                            maybeTriggerStatInterrupt(INT_STAT_OAM);
                            activeStatFlags &= ~INT_STAT_VBLANK;
                        } else {
                            regs.LY.val++;
                        }
                        checkLYC();
                    }
                    break;
            }
        }
    }
}

static const Color paletteColor[4] = {
    WHITE,
    LIGHTGRAY,
    DARKGRAY,
    BLACK
};

/*
static const Color screenPaletteColor[4] = {
    (Color){ 155, 188, 15, 255 },
    (Color){ 139, 172, 15, 255 },
    (Color){ 48,  98,  48, 255 },
    (Color){ 15,  56,  15, 255 }
};
*/
static const Color screenPaletteColor[4] = {
    (Color){ 175, 203, 70,  255 },
    (Color){ 121, 170, 109, 255 },
    (Color){ 34,  111, 95,  255 },
    (Color){ 8,   41,  85,  255 }
};

static void guiRegenTileTex(int index, Tile *tile)
{
    // Rendering tile with all three palettes to the same texture
    // Then when drawing the tile, the appropriate portion of the texture can be selected
    //  based on the desired palette in use.
    ImageClearBackground(&tileTextures[index].image, BLANK);
    for( int y = 0; y < 8; y++ ) {
        for( int x = 0; x < 8; x++ ) {
            int pixelPalIdx = (BIT(tile->line[y].hBits, 7-x) << 1) | BIT(tile->line[y].lBits, 7-x);
            int palColor = PALETTE_COLOR(regs.BGP.val, pixelPalIdx);
            ImageDrawPixel(&tileTextures[index].image, x, y, paletteColor[palColor]);
            if( 0 != pixelPalIdx ) {
                palColor = PALETTE_COLOR(regs.OBP0.val, pixelPalIdx);
                ImageDrawPixel(&tileTextures[index].image, x+8, y, paletteColor[palColor]);
                palColor = PALETTE_COLOR(regs.OBP1.val, pixelPalIdx);
                ImageDrawPixel(&tileTextures[index].image, x+16, y, paletteColor[palColor]);
            }
        }
    }
    UnloadTexture(tileTextures[index].tex);
    tileTextures[index].tex = LoadTextureFromImage(tileTextures[index].image);

}

static void guiRegenDirtyTiles(void)
{
    for(int i=0; i<384; i++) {
        if(true == tileTextures[i].dirty) {
            guiRegenTileTex(i, (Tile *)&vram.tiles[i]);
            tileTextures[i].dirty = false;
        }
    }
}

static void guiDrawTile2(const Vector2 anchor, int index, bool xFlip, bool yFlip, uint8_t palette, float scale)
{
    Rectangle source = { (float)8*palette, 0, 8, 8 };
    if( xFlip ) { source.width = -source.width; }
    if( yFlip ) { source.height = -source.height; }
    Rectangle dest = { anchor.x, anchor.y, 8*scale, 8*scale};
    Vector2 origin = { 0.0f, 0.0f };
    DrawTexturePro(tileTextures[index].tex, source, dest, origin, 0, WHITE);
}

static void guiDrawMapFrame(const Vector2 anchor, uint16_t x, uint16_t y, Color color)
{
    float left, right, top, bottom;
    //bottom := (SCY + 143) % 256 and right := (SCX + 159) % 256
    left = anchor.x + x;
    right = anchor.x + ((x + 160) % 256);
    top = anchor.y + y;
    bottom = ((y + 144) % 256) + anchor.y;
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

Vector2 guiDrawDisplayObjects(const Vector2 anchor)
{
    // WxH 256+8 x 256+16
    DrawRectangle(anchor.x, anchor.y, 256+8, 256+16, WHITE);
    DrawRectangle(anchor.x+8, anchor.y+16, SCREEN_WIDTH, SCREEN_HEIGHT, paletteColor[regs.BGP.palCol0]);
    for( int i=0; i < OAM_ENTRIES; i++ ) {
        OamEntry *object = &oamRam.entries[i];

        Vector2 tileAnchor = { anchor.x + object->xPos, anchor.y + object->yPos };
        PaletteReg pal = (object->attributes.palette)? regs.OBP0: regs.OBP1;
        if( 0 == regs.LCDC.objSize ) {
            guiDrawTile2(tileAnchor, object->tileIndex,
                        object->attributes.xFlip, object->attributes.yFlip,
                        1+object->attributes.palette, 1);
        } else {
            guiDrawTile2(tileAnchor, (object->tileIndex & 0xFE),
                        object->attributes.xFlip, object->attributes.yFlip,
                        1+object->attributes.palette, 1);
            tileAnchor.y += 8;
            guiDrawTile2(tileAnchor, (object->tileIndex & 0xFE)+1,
                        object->attributes.xFlip, object->attributes.yFlip,
                        1+object->attributes.palette, 1);
        }
    }
    DrawRectangle(anchor.x, anchor.y,                   256+8, 16, ColorAlpha(GRAY,0.3));
    DrawRectangle(anchor.x, anchor.y+SCREEN_HEIGHT+16,  256+8, 256-SCREEN_HEIGHT, ColorAlpha(GRAY,0.3));
    DrawRectangle(anchor.x, anchor.y+16,                8, SCREEN_HEIGHT, ColorAlpha(GRAY,0.3));
    DrawRectangle(anchor.x+SCREEN_WIDTH+8, anchor.y+16, 256-SCREEN_WIDTH, SCREEN_HEIGHT, ColorAlpha(GRAY,0.3));

    return (Vector2){256+8, 256+16};
}


Vector2 guiDrawOamEntry(const Vector2 viewAnchor, int index)
{
    OamEntry entry = oamRam.entries[index];
    Vector2 anchor = viewAnchor;

    anchor.x += 1;
    anchor.y += FONTSIZE*0.7f;
    DrawTextEx(firaFont, TextFormat("%02X", index), anchor, FONTSIZE, 0, BLACK);

    anchor.x += FONTWIDTH*3;
    anchor.y = viewAnchor.y;
    Vector2 size = guiDrawRegField(anchor, 2, "X", TextFormat("%02X", entry.xPos));

    anchor.x += size.x+FONTWIDTH;
    size = guiDrawRegField(anchor, 2, "Y", TextFormat("%02X", entry.yPos));

    anchor.x += size.x+FONTWIDTH;
    size = guiDrawRegField(anchor, 2, "TILE", TextFormat("%02X", entry.tileIndex));

    anchor.x += size.x+FONTWIDTH;
    if( 0 == regs.LCDC.objSize ) {
        anchor.y = viewAnchor.y+6;
        DrawRectangleV(anchor, (Vector2){2*8,2*8}, paletteColor[regs.BGP.palCol0]);
        guiDrawTile2(anchor, entry.tileIndex,
                    entry.attributes.xFlip, entry.attributes.yFlip,
                    1+entry.attributes.palette, 2);
        anchor.x += 16+FONTWIDTH;
    } else {
        anchor.x += 4;
        anchor.y = viewAnchor.y+6;
        DrawRectangleV(anchor, (Vector2){8,2*8}, paletteColor[regs.BGP.palCol0]);
        guiDrawTile2(anchor, (entry.tileIndex & 0xFE),
                    entry.attributes.xFlip, entry.attributes.yFlip,
                    1+entry.attributes.palette, 1);
        anchor.y += 8;
        guiDrawTile2(anchor, (entry.tileIndex & 0xFE)+1,
                    entry.attributes.xFlip, entry.attributes.yFlip,
                    1+entry.attributes.palette, 1);
        anchor.x += 12+FONTWIDTH;
    }

    anchor.y = viewAnchor.y;
    size = guiDrawHexReg(anchor, {5, {{"PRI",1},{"YFLP",1},{"XFLP",1},{"PAL",1},{"RSVD",4},}}, entry.attributes.val);

    anchor.x += size.x;
    return (Vector2){anchor.x-viewAnchor.x, size.y};
}

Vector2 guiDrawDisplayTileMap(const Vector2 anchor, const uint8_t map)
{
    // Width x Height = 256 x 256 or 288 x 288
    Vector2 tileAnchor = anchor;
    uint8_t tileRef;
    Tile *tile;

    for( int y = 0; y < 32; y++ ) {
        tileAnchor.x = anchor.x;
        for( int x = 0; x < 32; x++ ) {
            tileRef = vram.tileMap[map].tileRef[y][x];
            if( 1 == regs.LCDC.bgWinTileData ) {
                tile = (Tile *)&vram.tiles[tileRef];
                guiDrawTile2(tileAnchor, tileRef, false, false, 0, 1);
            } else {
                tile = (Tile *)&vram.tiles[ (256+((int8_t)tileRef))*16 ];
                guiDrawTile2(tileAnchor, (256+(int8_t)tileRef), false, false, 0, 1);
            }
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
            if(regs.WX.val < SCREEN_WIDTH && regs.WY.val < SCREEN_HEIGHT) {
                    // draw window frame
                if( regs.WX.val < 7 ) {
                    DrawRectangle(anchor.x + (7-regs.WX.val), anchor.y, SCREEN_WIDTH, SCREEN_HEIGHT-regs.WY.val, ColorAlpha(BLUE,0.3));
                } else {
                    DrawRectangle(anchor.x, anchor.y, SCREEN_WIDTH-(regs.WX.val-7), SCREEN_HEIGHT-regs.WY.val, ColorAlpha(BLUE,0.3));
                }
            }
        }
    }
    return (Vector2){tileAnchor.x-anchor.x, tileAnchor.y-anchor.y};
}

static Vector2 guiDrawPalette(const Vector2 anchor, const PaletteReg palette, const char *name, bool *active)
{
    Rectangle bounds = (Rectangle){anchor.x, anchor.y+8, 16*5, 24};
    Vector2 mousePoint = GetMousePosition();
    int fillStyle;
    bool fillSelector;

    // Check toggle button state
    if (CheckCollisionPointRec(mousePoint, bounds))
    {
        fillSelector = true;
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            fillStyle = GuiGetStyle(TOGGLE, BASE_COLOR_PRESSED);
        } else if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) {
            fillStyle = GuiGetStyle(TOGGLE, BASE_COLOR_NORMAL);
            *active = !(*active);
        } else {
            fillStyle = GuiGetStyle(TOGGLE, BASE_COLOR_FOCUSED);
        }
    } else {
        fillSelector = *active;
        fillStyle = GuiGetStyle(TOGGLE, BASE_COLOR_PRESSED);
    }

    if(fillSelector) {
        DrawRectangle(anchor.x, anchor.y+8, 16*5, 24, GetColor(fillStyle));
    }
    GuiGroupBox(bounds, name);
    for( int palIdx = 0; palIdx < 4; palIdx++ ) {
        int palColor = PALETTE_COLOR(palette.val, palIdx);
        DrawRectangle(anchor.x+8+16*(3-palIdx), anchor.y+12, 16, 16, paletteColor[palColor]);
    }
    return (Vector2){16*5,16*2};
}

static Vector2 guiDrawPaletteSelector(const Vector2 viewAnchor, int *selected)
{
    Vector2 anchor = viewAnchor;
    bool active = false;

    active = (0 == *selected);
    Vector2 size = guiDrawPalette(anchor, regs.BGP, "BGP", &active);
    *selected = (active)? 0 : *selected;

    active = (1 == *selected);
    anchor.x += size.x + GUI_PAD;
    size = guiDrawPalette(anchor, regs.OBP0, "OBP0", &active);
    *selected = (active)? 1 : *selected;

    active = (2 == *selected);
    anchor.x += size.x + GUI_PAD;
    size = guiDrawPalette(anchor, regs.OBP1, "OPB1", &active);
    *selected = (active)? 2 : *selected;

    anchor.x += size.x;

    return (Vector2){anchor.x - viewAnchor.x, size.y};
}

Vector2 guiDrawDisplayTileData(const Vector2 anchor)
{
    // Width x Height = 18*17 x 25*17 = 306 x 425

    uint16_t index = 0;
    Vector2 tileAnchor = anchor;

    guiRegenDirtyTiles();

    const Color lineColor = GetColor(GuiGetStyle(DEFAULT,LINE_COLOR));

    static int selectedPalette = 0;
    guiDrawPaletteSelector((Vector2){anchor.x+FONTWIDTH*3, anchor.y+FONTSIZE+24*(2*8+1)}, &selectedPalette);

    // Draw offsets across the top
    tileAnchor.x += 4 + FONTWIDTH*2;
    for( int x = 0; x < 16; x++ ) {
        DrawTextEx(firaFont, TextFormat("%X",x), tileAnchor, FONTSIZE, 0, lineColor);
        tileAnchor.x += 2*8+1;
    }

    // Draw Tile Map itself
    tileAnchor = (Vector2){ anchor.x, anchor.y + FONTSIZE };
    for( int y = 0; y < 24; y++ ) {
        tileAnchor.x = anchor.x;

        // Draw indexes for selected BG/Win tile data
        // The s
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
            if(0 != selectedPalette) {
                DrawRectangleV(tileAnchor, (Vector2){2*8,2*8}, paletteColor[regs.BGP.palCol0]);
            }
            guiDrawTile2(tileAnchor, index, false, false, selectedPalette, 2);
            index++;
            tileAnchor.x += 2*8+1;
        }

        // Draw indexes for Obj tiles
        if( y < 16 ) {
            DrawTextEx(firaFont, TextFormat("%X0",y), tileAnchor, FONTSIZE, 0, lineColor);
        }
        tileAnchor.y += 2*8+1;
    }
    return (Vector2){(tileAnchor.x+FONTWIDTH*2)-anchor.x, tileAnchor.y-anchor.y+32};
}

Vector2 guiDrawDisplayScreen(const Vector2 anchor)
{
    DrawRectangleV(anchor, (Vector2){ SCREEN_WIDTH*3, SCREEN_HEIGHT*3 }, ColorAlpha(screenPaletteColor[0], 0.7));
    if(1 == regs.LCDC.displayEnable) {
        //DrawRectangleV(anchor, (Vector2){ 160*3, 144*3 }, screenPaletteColor[0]);
        Rectangle pixelRect = { anchor.x, anchor.y, 2.6, 2.6 };
        for( int y = 0; y < SCREEN_HEIGHT; y++ ) {
            pixelRect.x = anchor.x;
            for( int x = 0; x < SCREEN_WIDTH; x++ ) {
                int palColor = screenData[y][x];
                DrawRectangleRec(pixelRect, screenPaletteColor[palColor]);
                pixelRect.x += 2+1;
            }
            pixelRect.y += 2+1;
        }
    }
    guiUpdateScreen = false;

    return (Vector2){SCREEN_WIDTH*3, SCREEN_HEIGHT*3};
}


Vector2 guiDrawDisplay(const Vector2 viewAnchor)
{
    Vector2 anchor = viewAnchor;

    // Main Display
    Vector2 size = guiDrawDisplayScreen(viewAnchor);
    anchor.x += size.x + GUI_PAD;

    size = guiDrawDisplayTileData(anchor); // (Vector2){850, 50});
    anchor.x += size.x + GUI_PAD;
    anchor.y = viewAnchor.y + FONTSIZE;

    size = guiDrawDisplayTileMap(anchor, 0); // (Vector2){550, 50}, 0);
    anchor.y += size.y + GUI_PAD;

    size = guiDrawDisplayTileMap(anchor, 1); // (Vector2){550, 350}, 1);
    anchor.x += size.x + GUI_PAD;
    anchor.y = viewAnchor.y + 16;

    size = guiDrawDisplayObjects(anchor);
    anchor.y += size.y + GUI_PAD;

    return (Vector2){0,0};
}

const RegViewList displayRegView = {
    13,
    NULL,
    {
        { &regs.LCDC.val, "LCDC", "FF40", {8, {{"EN",1},{"wMAP",1},{"wEN",1},{"bTIL",1},{"bMAP",1},{"obSIZ",1},{"obEN",1},{"bwEN",1}}} },
        { &regs.STAT.val, "STAT", "FF41", {7, {{"RSVD",1},{"lycIE",1},{"oamIE",1},{"vblIE",1},{"hblIE",1},{"lyEQ",1},{"MODE",2},}}},
        { &regs.SCY.val,  "SCY",  "FF42", {1, {{"SCY", 8},}} },
        { &regs.SCX.val,  "SCX",  "FF43", {1, {{"SCX", 8},}} },
        { &regs.LY.val,   "LY",   "FF44", {1, {{"LY", 8},}} },
        { &regs.LYC.val,  "LYC",  "FF45", {1, {{"LYC", 8},}} },
        { &regs.OAM.val,  "OAM",  "FF46", {1, {{"OAM", 8},}} },
        { &oamDmaOffset,  "oam", "state", {1, {{"offset", 8},}} },
        { &regs.BGP.val,  "BGP",  "FF47", {4, {{"COL3",2},{"COL2",2},{"COL1",2},{"COL0",2},}}},
        { &regs.OBP0.val, "OBP0", "FF48", {4, {{"COL3",2},{"COL2",2},{"COL1",2},{"COL0",2},}}},
        { &regs.OBP1.val, "OBP1", "FF49", {4, {{"COL3",2},{"COL2",2},{"COL1",2},{"COL0",2},}}},
        { &regs.WY.val,   "WY",   "FF4A", {1, {{"WY", 8},}} },
        { &regs.WX.val,   "WX",   "FF4B", {1, {{"WX", 8},}} },
    }
};

const RegViewList oamRegView = {
    40,
    guiDrawOamEntry,
    {}
};

void displayInit(void)
{
    memset(&regs, 0, sizeof(regs));
    frameCounter = 0;
    scanlineCounter = 0;
    totalFrames = 0;
    activeStatFlags = 0;
    memset(&vram, 0, sizeof(vram));
    vramImage.size = 0x2000;
    vramImage.contents = vram.contents;
    addRamView(&vramImage, "VRAM", 0x8000);
    guiUpdateScreen = false;
    memset(&tileTextures, 0, sizeof(tileTextures));
    // setup initial blank tile textures
    for(int i=0; i<384; i++) {
        tileTextures[i].image = GenImageColor(8*3, 8, BLANK);
        tileTextures[i].tex = LoadTextureFromImage(tileTextures[i].image);
    }
    memset(screenData, 0, sizeof(screenData));
    bgFetch.reset(true, false);
    objFetch.reset(true);
    oamImage.size = OAM_SIZE;
    oamImage.contents = oamRam.contents;
    addRamView(&oamImage, "OAM", 0xFE00);
    oamDmaOffset = OAM_SIZE;
    regs.OAM.val = 0xFF;    // reset value

    addRegView(&displayRegView, "DISP");
    addRegView(&oamRegView, "OAM");
}

void displayDeinit(void)
{
    // TODO: unload textures
}

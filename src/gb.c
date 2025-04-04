// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#include "gb.h"

unsigned int mainClock = 0;

RomImage bootrom;
bool bootRomActive = true;

extern RamImage vram;
RamImage wram;
RamImage hram;


void gbInit(const char * const cartFilename)
{
    mainClock = 0;

    memInit();

    printf("Loading Boot ROM...");
    if(SUCCESS != loadRom(&bootrom, "Resources/ROMs/DMG_ROM.bin", BOOTROM_ENTRY)) {
        printf("ERROR\n");
        exit(1);
    }
    //preprocessRom(&bootrom, BOOTROM_ENTRY);
    addRomView(&bootrom, "BOOT", 0x0000);
    printf("SUCCESS\n");

    if( SUCCESS != loadCartridge(cartFilename)) {
        exit(0);
    }

    resetCpu();
    controlsInit();
    serialInit();
    timerInit();
    displayInit();
    audioInit();

    allocateRam(&wram, 8192);
    addRamView(&wram, "WRAM", 0xC000);
    allocateRam(&hram, 0x80);
    addRamView(&hram, "HRAM", 0xFF80);
}

void gbDeinit(void)
{
    unloadRom(&bootrom);
    unloadCartridge();
    deallocateRam(&wram);
    displayDeinit();
    audioDeinit();
}

void cpuCycle(void)
{
    timerTick();
    mainClock += MAIN_CLOCKS_PER_CPU_CYCLE;
    ppuCycles(MAIN_CLOCKS_PER_CPU_CYCLE);
    oamDmaCycle();
}

void cpuCycles(int cycles)
{
    while(cycles--) {
        cpuCycle();
    }
}

typedef struct {
    uint8_t (*getIo8)(uint16_t addr);
    void (*setIo8)(uint16_t addr, uint8_t val8);
} IoRegDispatchFuncs;

IoRegDispatchFuncs ioRegDispatch[0x78] = {
    { getControlsReg8, setControlsReg8 }, // FF00
    { getSerialReg8, setSerialReg8 }, // FF01 SB
    { getSerialReg8, setSerialReg8 }, // FF02 SC
    { NULL, NULL }, // FF03
    { getTimerReg8, setTimerReg8 }, // FF04 Timer DIV
    { getTimerReg8, setTimerReg8 }, // FF05 Timer TIMA
    { getTimerReg8, setTimerReg8 }, // FF06 Timer TMA
    { getTimerReg8, setTimerReg8 }, // FF07 Timer TAC
    { NULL, NULL }, // FF08
    { NULL, NULL }, // FF09
    { NULL, NULL }, // FF0A
    { NULL, NULL }, // FF0B
    { NULL, NULL }, // FF0C
    { NULL, NULL }, // FF0D
    { NULL, NULL }, // FF0E
    { getIntReg8, setIntReg8 }, // FF0F IF

    { getAudioReg8, setAudioReg8 }, // FF10 AUD CH1 SWEEP
    { getAudioReg8, setAudioReg8 }, // FF11 AUD CH1 TIMER
    { getAudioReg8, setAudioReg8 }, // FF12 AUD CH1 ENVLP
    { getAudioReg8, setAudioReg8 }, // FF13 AUD CH1 LPER
    { getAudioReg8, setAudioReg8 }, // FF14 AUD CH1 CTRL
    { NULL, NULL }, // FF15
    { getAudioReg8, setAudioReg8 }, // FF16 AUD CH2 TIMER
    { getAudioReg8, setAudioReg8 }, // FF17 AUD CH2 ENVLP
    { getAudioReg8, setAudioReg8 }, // FF18 AUD CH2 LPER
    { getAudioReg8, setAudioReg8 }, // FF19 AUD CH2 CTRL
    { getAudioReg8, setAudioReg8 }, // FF1A AUD CH3 DAC
    { getAudioReg8, setAudioReg8 }, // FF1B AUD CH3 TIMER
    { getAudioReg8, setAudioReg8 }, // FF1C AUD CH3 ENVLP
    { getAudioReg8, setAudioReg8 }, // FF1D AUD CH3 LPER
    { getAudioReg8, setAudioReg8 }, // FF1E AUD CH3 CTRL
    { NULL, NULL }, // FF1F

    { getAudioReg8, setAudioReg8 }, // FF20 AUD CH4 TIMER
    { getAudioReg8, setAudioReg8 }, // FF21 AUD CH4 ENVLP
    { getAudioReg8, setAudioReg8 }, // FF22 AUD CH4 LFSR
    { getAudioReg8, setAudioReg8 }, // FF23 AUD CH4 CTRL
    { getAudioReg8, setAudioReg8 }, // FF24 AUD MAST VOL
    { getAudioReg8, setAudioReg8 }, // FF25 AUD MAST PAN
    { getAudioReg8, setAudioReg8 }, // FF26 AUD MAST CTRL
    { NULL, NULL }, // FF27
    { NULL, NULL }, // FF28
    { NULL, NULL }, // FF29
    { NULL, NULL }, // FF2A
    { NULL, NULL }, // FF2B
    { NULL, NULL }, // FF2C
    { NULL, NULL }, // FF2D
    { NULL, NULL }, // FF2E
    { NULL, NULL }, // FF2F

    { getAudioReg8, setAudioReg8 }, // FF30 AUD WAV0
    { getAudioReg8, setAudioReg8 }, // FF31 AUD WAV1
    { getAudioReg8, setAudioReg8 }, // FF32 AUD WAV2
    { getAudioReg8, setAudioReg8 }, // FF33 AUD WAV3
    { getAudioReg8, setAudioReg8 }, // FF34 AUD WAV4
    { getAudioReg8, setAudioReg8 }, // FF35 AUD WAV5
    { getAudioReg8, setAudioReg8 }, // FF36 AUD WAV6
    { getAudioReg8, setAudioReg8 }, // FF37 AUD WAV7
    { getAudioReg8, setAudioReg8 }, // FF38 AUD WAV8
    { getAudioReg8, setAudioReg8 }, // FF39 AUD WAV9
    { getAudioReg8, setAudioReg8 }, // FF3A AUD WAVA
    { getAudioReg8, setAudioReg8 }, // FF3B AUD WAVB
    { getAudioReg8, setAudioReg8 }, // FF3C AUD WAVC
    { getAudioReg8, setAudioReg8 }, // FF3D AUD WAVD
    { getAudioReg8, setAudioReg8 }, // FF3E AUD WAVE
    { getAudioReg8, setAudioReg8 }, // FF3F AUD WAVF

    { getGfxReg8, setGfxReg8 }, // FF40 Gfx LCDC
    { getGfxReg8, setGfxReg8 }, // FF41 Gfx STAT
    { getGfxReg8, setGfxReg8 }, // FF42 Gfx SCY
    { getGfxReg8, setGfxReg8 }, // FF43 Gfx SCX
    { getGfxReg8, setGfxReg8 }, // FF44 Gfx LY
    { getGfxReg8, setGfxReg8 }, // FF45 Gfx LYC
    { getGfxReg8, setGfxReg8 }, // FF46 Gfx OAM DMA
    { getGfxReg8, setGfxReg8 }, // FF47 Gfx BGP
    { getGfxReg8, setGfxReg8 }, // FF48 Gfx OBP0
    { getGfxReg8, setGfxReg8 }, // FF49 Gfx OBP1
    { getGfxReg8, setGfxReg8 }, // FF4A Gfx WY
    { getGfxReg8, setGfxReg8 }, // FF4B Gfx WX
    { NULL, NULL }, // FF4C
    { NULL, NULL }, // FF4D
    { NULL, NULL }, // FF4E
    { NULL, NULL }, // FF4F

    { NULL, NULL }, // FF50
    { NULL, NULL }, // FF51
    { NULL, NULL }, // FF52
    { NULL, NULL }, // FF53
    { NULL, NULL }, // FF54
    { NULL, NULL }, // FF55
    { NULL, NULL }, // FF56
    { NULL, NULL }, // FF57
    { NULL, NULL }, // FF58
    { NULL, NULL }, // FF59
    { NULL, NULL }, // FF5A
    { NULL, NULL }, // FF5B
    { NULL, NULL }, // FF5C
    { NULL, NULL }, // FF5D
    { NULL, NULL }, // FF5E
    { NULL, NULL }, // FF5F

    { NULL, NULL }, // FF60
    { NULL, NULL }, // FF61
    { NULL, NULL }, // FF62
    { NULL, NULL }, // FF63
    { NULL, NULL }, // FF64
    { NULL, NULL }, // FF65
    { NULL, NULL }, // FF66
    { NULL, NULL }, // FF67
    { NULL, NULL }, // FF68
    { NULL, NULL }, // FF69
    { NULL, NULL }, // FF6A
    { NULL, NULL }, // FF6B
    { NULL, NULL }, // FF6C
    { NULL, NULL }, // FF6D
    { NULL, NULL }, // FF6E
    { NULL, NULL }, // FF6F

    { NULL, NULL }, // FF70
    { NULL, NULL }, // FF71
    { NULL, NULL }, // FF72
    { NULL, NULL }, // FF73
    { NULL, NULL }, // FF74
    { NULL, NULL }, // FF75
    { NULL, NULL }, // FF76
    { NULL, NULL }, // FF77
};

//0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
//4000	7FFF	16 KiB ROM Bank 01–NN	From cartridge, switchable bank via mapper (if any)
//8000	9FFF	8 KiB Video RAM (VRAM)	In CGB mode, switchable bank 0/1
//A000	BFFF	8 KiB External RAM	From cartridge, switchable bank if any
//C000	CFFF	4 KiB Work RAM (WRAM)
//D000	DFFF	4 KiB Work RAM (WRAM)	In CGB mode, switchable bank 1–7
//E000	FDFF	Echo RAM (mirror of C000–DDFF)	Nintendo says use of this area is prohibited.
//FE00	FE9F	Object attribute memory (OAM)
//FEA0	FEFF	Not Usable	Nintendo says use of this area is prohibited.
//FF00	FF7F	I/O Registers
//FF80	FFFE	High RAM (HRAM)
//FFFF	FFFF	Interrupt Enable register (IE)

// Raw version has no bus conflicts
uint8_t getRawMem8(uint16_t addr)
{
    if(addr < 0x00100 && bootRomActive) {
        return bootrom.contents[addr];

    } else if( addr < 0x7FFF ) {
        // ROM Bank 0-n
        return getCartRom8(addr&0x7FFF);

    } else if( addr >= 0x8000 && addr <= 0x9FFF) {
        // VRAM
        return getVram8(addr&0x1FFF);

    } else if( addr >= 0xA000 && addr <= 0xBFFF) {
        // CARTRIDGE RAM
        return getCartRam8(addr&0x1FFF);

    } else if( addr >= 0xC000 && addr <= 0xDFFF) {
        // WORK RAM
        return wram.contents[addr&0x1FFF];

    } else if( addr >= 0xE000 && addr <= 0xFDFF ) {
        // ECHO RAM
        return wram.contents[addr&0x1FFF];

    } else if( addr >= 0xFE00 && addr <= 0xFE9F ) {
        // OAM
        return getOam8(addr - 0xFE00);

    } else if( addr >= 0xFEA0 && addr <= 0xFEFF ) {
        // Prohibited
        return 0x42; // TODO

    } else if( addr >= 0xFF00 && addr <= 0xFF7F ) {
        // IO Regs
        if( 0xFF50 == addr ) {
            return (bootRomActive)? 0:1;
        } else if( addr <= 0xFF77 ) {
            if( NULL != ioRegDispatch[addr & 0x00FF].getIo8 ) {
                return ioRegDispatch[addr & 0x00FF].getIo8(addr);
            } else {
                return MISSING_REG_VAL;
            }
        } else {
            return UNMAPPED_REG_VAL;
        }
    } else if( addr >= 0xFF80 && addr <= 0xFFFE ) {
        // High RAM
        return hram.contents[addr&0x007F];

    } else /*( addr == 0xFFFF )*/ {
        // Interrupt Enable
        return getIntReg8(REG_IE_ADDR);
    }
}

uint8_t getMem8(uint16_t addr)
{
    if( addr >= 0xFE00 && addr <= 0xFE9F) {
        return (oamDmaActive())? 0xFF : getOam8(addr - 0xFE00);
    } else {
       return getRawMem8(addr);
    }
}

void setRawMem8(uint16_t addr, uint8_t val8)
{
    if( addr <= 0x7FFF ) {
        // ROM Bank 0-n
        setCartRom8(addr & 0x7FFF, val8);
    } else if( addr >= 0x8000 && addr <= 0x9FFF) {
        // VRAM
        setVram8(addr&0x1FFF, val8);

    } else if( addr >= 0xA000 && addr <= 0xBFFF) {
        // CARTRIDGE RAM
        setCartRam8(addr&0x1FFF, val8);

    } else if( addr >= 0xC000 && addr <= 0xDFFF) {
        // WORK RAM
        wram.contents[addr&0x1FFF] = val8;

    } else if( addr >= 0xE000 && addr <= 0xFDFF ) {
        // ECHO RAM
        wram.contents[addr&0x1FFF] = val8;

    } else if( addr >= 0xFE00 && addr <= 0xFE9F ) {
        // OAM
        setOam8(addr - 0xFE00, val8);

    } else if( addr >= 0xFEA0 && addr <= 0xFEFF ) {
        // Prohibited
        return; // TODO

    } else if( addr >= 0xFF00 && addr <= 0xFF7F ) {
        // IO Regs
        if( 0xFF50 == addr ) {
            bootRomActive = (0 == val8);
        } else if( addr <= 0xFF77 ) {
            if( NULL != ioRegDispatch[addr & 0x00FF].setIo8 ) {
                ioRegDispatch[addr & 0x00FF].setIo8(addr, val8);
            }
        }
    } else if( addr >= 0xFF80 && addr <= 0xFFFE ) {
        // High RAM
        hram.contents[addr&0x007F] = val8;

    } else /*if( addr == 0xFFFF )*/ {
        // Interrupt Enable
        setIntReg8(REG_IE_ADDR, val8);
    }
}

void setMem8(uint16_t addr, uint8_t val8)
{
    if( !oamDmaActive()) {
        setRawMem8(addr, val8);
    }
}

uint8_t readMem8(uint16_t addr)
{
    uint8_t val8 = getMem8(addr);
    cpuCycle();
    return val8;
}

void writeMem8(uint16_t addr, uint8_t val8)
{
    setMem8(addr, val8);
    cpuCycle();
}

uint16_t readMem16(uint16_t addr)
{
    uint16_t val16 = readMem8(addr);
    val16 |= readMem8(addr+1)<<8;
    return val16;
}

void writeMem16(uint16_t addr, uint16_t val16)
{
    writeMem8(addr,(val16&0x00ff));
    writeMem8(addr+1,(val16&0xff00)>>8);
}

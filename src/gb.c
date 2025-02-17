#include "gb.h"

unsigned int mainClock = 0;

RomImage bootrom;
bool bootRomActive = true;

Cartridge cartridge = {0};
bool cartridgeInserted = false;

RamImage wram;
RamImage vram;
RamImage hram;

uint8_t getNoCartRom8(uint16_t addr){
    return 0xAA;
}

void gbInit(const char * const cartFilename)
{
    mainClock = 0;

    printf("Loading Boot ROM...");
    if(SUCCESS != loadRom(&bootrom, "Resources/ROMs/DMG_ROM.bin", BOOTROM_ENTRY)) {
        printf("ERROR\n");
        exit(1);
    }
    preprocessRom(&bootrom, BOOTROM_ENTRY);
    addRomView(&bootrom, "BOOT", 0x0000);
    printf("SUCCESS\n");

    if( NULL != cartFilename ) {
        if( SUCCESS != loadCartridge(&cartridge, cartFilename)) {
            exit(0);
        }
        addRomView(&cartridge.rom, "CART", 0x0000);
        cartridgeInserted = true;
    } else {
        cartridge.getCartMappedRom8 = getNoCartRom8;
    }


    allocateRam(&vram, 8192);
    addRamView(&vram, "VRAM", 0x8000);
    allocateRam(&wram, 8192);
    addRamView(&wram, "WRAM", 0xC000);
    allocateRam(&hram, 0x80);
    addRamView(&hram, "HRAM", 0xFF80);

    resetCpu();
}

void gbDeinit(void)
{
    unloadRom(&bootrom);
    if(cartridgeInserted) {
        unloadCartridge(&cartridge);
    }
    deallocateRam(&wram);
    deallocateRam(&vram);

}
void cpuCycles(int cycles)
{
    mainClock +=MAIN_CLOCKS_PER_CPU_CYCLE;
}


uint8_t getCartBank0Mem8(uint16_t addr)
{
    if (cartridgeInserted) {
        return cartridge.rom.contents[addr];
    } else {
        return getNoCartRom8(addr);
    }
}

uint8_t getCartMem8(uint16_t addr)
{
    return cartridge.getCartMappedRom8(addr);
}

//0000	3FFF	16 KiB ROM bank 00	From cartridge, usually a fixed bank
//4000	7FFF	16 KiB ROM Bank 01–NN	From cartridxge, switchable bank via mapper (if any)
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

uint8_t getMem8(uint16_t addr)
{
    switch( (addr&0xF000) >> 12 ) {
        case 0x0:  // ROM Bank 0
            if(addr < 0x00100 && bootRomActive) {
                return bootrom.contents[addr];
            } else {
                return getCartBank0Mem8(addr);
            }
        case 0x1:   // ROM Bank 0
        case 0x2:   // ROM Bank 0
        case 0x3:   // ROM Bank 0
            return getCartBank0Mem8(addr);
        case 0x4:   // ROM Bank n
        case 0x5:   // ROM Bank n
        case 0x6:   // ROM Bank n
        case 0x7:   // ROM Bank n
            return getCartMem8(addr);
        case 0x8:   // VRAM
        case 0x9:   // VRAM
            return vram.contents[addr&0x1FFF];
        case 0xA:   // CARTRIDGE RAM
        case 0xB:   // CARTRIGDE RAM
            return 0xA;
        case 0xC:   // WORK RAM
        case 0xD:   // WORK RAM
            return wram.contents[addr&0x1FFF];
        case 0xE:   // ECHO RAM
        case 0xF:   // MISC
            if( addr >= 0xE000 && addr <= 0xFDFF ) {
                // ECHO RAM
                return wram.contents[addr&0x1FFF];
            } else if( addr >= 0xFE00 && addr <= 0xFE9F ) {
                // OAM
                return 0x42; // TODO
            } else if( addr >= 0xFEA0 && addr <= 0xFEFF ) {
                // Prohibited
                return 0x42; // TODO
            } else if( addr >= 0xFF00 && addr <= 0xFF7F ) {
                // IO Regs
                if( 0xFF50 == addr ) {
                    return (bootRomActive)? 0:1;
                }
                return 0x90; // TODO
            } else if( addr >= 0xFF80 && addr <= 0xFFFE ) {
                // High RAM
                return hram.contents[addr&0x007F];
            } else if( addr == 0xFFFF ) {
                // Interrupt Enable
                return 0x42; // TODO
            }
    }
}

uint8_t sb, sc;

void setMem8(uint16_t addr, uint8_t val8)
{
    switch( (addr&0xF000) >> 12 ) {
        case 0x0:   // ROM Bank 0
        case 0x1:   // ROM Bank 0
        case 0x2:   // ROM Bank 0
        case 0x3:   // ROM Bank 0
            return;
        case 0x4:   // ROM Bank n
        case 0x5:   // ROM Bank n
        case 0x6:   // ROM Bank n
        case 0x7:   // ROM Bank n
            return;
        case 0x8:   // VRAM
        case 0x9:   // VRAM
            vram.contents[addr&0x1FFF]=val8;
            return;
        case 0xA:   // CARTRIDGE RAM
        case 0xB:   // CARTRIDGE RAM
            return;
        case 0xC:   // WORK RAM
        case 0xD:   // WORK RAM
            wram.contents[addr&0x1FFF]=val8;
            return;
        case 0xE:   // ECHO RAM
        case 0xF:   // MISC
            if( addr >= 0xE000 && addr <= 0xFDFF ) {
                // ECHO RAM
                wram.contents[addr&0x1FFF]=val8;
                return;
            } else if( addr >= 0xFE00 && addr <= 0xFE9F ) {
                // OAM
                return; // TODO
            } else if( addr >= 0xFEA0 && addr <= 0xFEFF ) {
                // Prohibited
                return; // TODO
            } else if( addr >= 0xFF00 && addr <= 0xFF7F ) {
                // IO Regs
                if( 0xFF01 == addr ) {
                    sb = val8;
                } else if (0xFF02 == addr) {
                    if( val8 & 0x80 ) {
                        printf("%c", sb);
                    }
                } else if( 0xFF50 == addr ) {
                    bootRomActive = (0 == val8);
                }
                return; // TODO
            } else if( addr >= 0xFF80 && addr <= 0xFFFE ) {
                // High RAM
                hram.contents[addr&0x007F] = val8;
            } else if( addr == 0xFFFF ) {
                // Interrupt Enable
                return; // TODO
            }
    }
}

uint8_t readMem8(uint16_t addr)
{
    uint8_t val8 = getMem8(addr);
    cpuCycles(1);
    return val8;
}

void writeMem8(uint16_t addr, uint8_t val8)
{
    setMem8(addr, val8);
    cpuCycles(1);
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

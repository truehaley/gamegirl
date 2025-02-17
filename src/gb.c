#include "gb.h"

unsigned int mainClock = 0;

RomImage bootrom;
bool bootRomActive = true;

RamImage wram;
RamImage vram;
RamImage hram;


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

    if( SUCCESS != loadCartridge(cartFilename)) {
        exit(0);
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
    unloadCartridge();
    deallocateRam(&wram);
    deallocateRam(&vram);

}
void cpuCycles(int cycles)
{
    mainClock +=MAIN_CLOCKS_PER_CPU_CYCLE;
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
    if(addr < 0x00100 && bootRomActive) {
        return bootrom.contents[addr];

    } else if( addr < 0x7FFF ) {
        // ROM Bank 0-n
        return getCartRom8(addr&0x7FFF);

    } else if( addr >= 0x8000 && addr <= 0x9FFF) {
        // VRAM
        return vram.contents[addr&0x1FFF];

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

    } else /*( addr == 0xFFFF )*/ {
        // Interrupt Enable
        return 0x42; // TODO
    }
}

uint8_t sb, sc;

void setMem8(uint16_t addr, uint8_t val8)
{
    if( addr < 0x7FFF ) {
        // ROM Bank 0-n
        setCartRom8(addr & 0x7FFF, val8);
    } else if( addr >= 0x8000 && addr <= 0x9FFF) {
        // VRAM
        vram.contents[addr&0x1FFF] = val8;

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
        return; // TODO

    } else if( addr >= 0xFEA0 && addr <= 0xFEFF ) {
        // Prohibited
        return; // TODO

    } else if( addr >= 0xFF00 && addr <= 0xFF7F ) {
        // IO Regs
        // TODO
        if( 0xFF01 == addr ) {
            sb = val8;
        } else if (0xFF02 == addr) {
            if( val8 & 0x80 ) {
                printf("%c", sb);
            }
        } else if( 0xFF50 == addr ) {
            bootRomActive = (0 == val8);
        }

    } else if( addr >= 0xFF80 && addr <= 0xFFFE ) {
        // High RAM
        hram.contents[addr&0x007F] = val8;

    } else if( addr == 0xFFFF ) {
        // Interrupt Enable
        // TODO

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

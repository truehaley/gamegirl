/*
Raylib example file.
This is an example main file for a simple raylib project.
Use this as a starting point or replace it with your code.

by Jeffery Myers is marked with CC0 1.0. To view a copy of this license, visit https://creativecommons.org/publicdomain/zero/1.0/

*/

#include "cartridge.h"
#include "raylib.h"
#include "raygui.h"
#include "resource_dir.h"// utility header for SearchAndSetResourceDir
#include "mem.h"
#include "gui.h"

#include <math.h>

#include <stdlib.h>
#include <stdio.h>


RomImage bootrom;
Cartridge cartridge = {0};
RamImage wram;
RamImage vram;


int main(int argc, const char *argv[])
{
    // Utility function from resource_dir.h to find the resources folder and set it as the current working directory so we can load from it
    //SearchAndSetResourceDir("resources");

    printf("Loading Boot ROM...");
    if(SUCCESS != loadRom(&bootrom, "Resources/ROMs/DMG_ROM.bin", BOOTROM_ENTRY)) {
        printf("ERROR\n");
        exit(1);
    }
    preprocessRom(&bootrom, BOOTROM_ENTRY);
    addRomView(&bootrom, "BOOT");
    printf("SUCCESS\n");

    if( argc > 1 ) {
        loadCartridge(&cartridge, argv[1]);
        addRomView(&cartridge.rom, "CART");
        //dumpMemory(cartridge.rom.contents, cartridge.rom.size);
        //disassembleRom(&cartridge.rom);
        //unloadCartridge(&cartridge);
    } else {
        //dumpMemory(bootrom.contents, bootrom.size);
        //disassembleRom(&bootrom);
    }

    allocateRam(&wram, 8192);
    addRamView(&wram, "WRAM");
    allocateRam(&vram, 8192);
    addRamView(&vram, "VRAM");

    gui();
    unloadRom(&bootrom);
    if(NULL != cartridge.rom.contents) {
        unloadCartridge(&cartridge);
    }
    deallocateRam(&wram);
    deallocateRam(&vram);
    return 0;
}

#include "gb.h"
#include "gui.h"


int main(int argc, const char *argv[])
{
    if(argc > 1) {
        gbInit(argv[1]);
    } else {
        gbInit(NULL);
    }

    /*
    if( argc > 1 ) {
        //dumpMemory(cartridge.rom.contents, cartridge.rom.size);
        //disassembleRom(&cartridge.rom);
        //unloadCartridge(&cartridge);
    } else {
        //dumpMemory(bootrom.contents, bootrom.size);
        //disassembleRom(&bootrom);
        }*/


    gui();

    gbDeinit();
    return 0;
}

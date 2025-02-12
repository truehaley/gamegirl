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

#include "cpu.h"

#include <stdlib.h>
#include <stdio.h>

void gui(void) {
    // Tell the window to use vsync and work on high DPI displays
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    // Create the window and OpenGL context
    InitWindow(1280, 800, "GameGirl");
    SetTargetFPS(60);


    Vector2 InterfaceAnchor = { 8, 8 };
    int pressed;

    // Load a texture from the resources directory
    Texture wabbit = LoadTexture("wabbit_alpha.png");

    // game loop
    while (!WindowShouldClose())// run the loop untill the user presses ESCAPE or presses the Close button on the window
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Implement required update logic
        //----------------------------------------------------------------------------------

        // drawing
        BeginDrawing();

            // Setup the back buffer for drawing (clear color and depth buffers)
            //ClearBackground(BLACK);
            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

            // draw some text using the default font
            DrawText("Hello Raylib", 100,300,20,WHITE);

            GuiPanel((Rectangle){ InterfaceAnchor.x, InterfaceAnchor.y, 336, 360 }, NULL);
            GuiPanel((Rectangle){ InterfaceAnchor.x + 8, InterfaceAnchor.y + 8, 320, 288 }, NULL);
            pressed = GuiButton((Rectangle){ InterfaceAnchor.x + 40, InterfaceAnchor.y + 328, 24, 24 }, GuiIconText(ICON_ARROW_LEFT_FILL, NULL));

            // draw our texture to the screen
            DrawTexture(wabbit, 400, 200, WHITE);

        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }

    // cleanup
    // unload our texture so it can be cleaned up
    UnloadTexture(wabbit);

    // destroy the window and cleanup the OpenGL context
    CloseWindow();

}

RomImage bootrom;
Cartridge cartridge;


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
    printf("SUCCESS\n");

    if( argc > 1 ) {
        loadCartridge(&cartridge, argv[1]);
        //dumpMemory(cartridge.rom.contents, cartridge.rom.size);
        disassembleRom(&cartridge.rom);
        unloadCartridge(&cartridge);
    } else {
        dumpMemory(bootrom.contents, bootrom.size);
        disassembleRom(&bootrom);
    }

    unloadRom(&bootrom);
    return 0;
}

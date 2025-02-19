#include "gui.h"
#include "cpu.h"
#include "gb.h"
#include "raylib.h"

Font firaFont;
uint16_t systemBreakpoint = 0xFFFF;

void gui(void)
{
    // Tell the window to use vsync and work on high DPI displays
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

    // Create the window and OpenGL context
    InitWindow(1280, 800, "GameGirl");
    SetTargetFPS(60);


    Vector2 InterfaceAnchor = { 8, 8 };
    int pressed;

    // Load a texture from the resources directory
    Texture wabbit = LoadTexture("Resources/wabbit_alpha.png");
    firaFont = LoadFontEx("resources/Fonts/FiraMono/FiraMonoNerdFont-Regular.otf", FONTSIZE, 0, 250);

    int instructionsPerTab = 10000;
    bool keepRunning = true;

    // game loop
    // run the loop untill the user presses ESCAPE or presses the Close button on the window
    while (!WindowShouldClose() && keepRunning)
    {
        // Update
         if(IsKeyPressed(KEY_SPACE)) {
            executeInstruction(systemBreakpoint);
            running = false;
        }
        if(IsKeyPressed(KEY_TAB) | running) {
            for(int i=0; i<instructionsPerTab; i++) {
                if( executeInstruction(systemBreakpoint) ) {
                    running = false;
                    if( exitOnBreak ) {
                        keepRunning = false;
                    }
                    break;
                }
            }
        }
        if(IsKeyPressed(KEY_UP)) {
            instructionsPerTab+= (IsKeyDown(KEY_LEFT_SHIFT))?100:10;
        }
        if(IsKeyPressed(KEY_DOWN)) {
            instructionsPerTab-= (IsKeyDown(KEY_LEFT_SHIFT))?100:10;
        }
        if(IsKeyPressed(KEY_LEFT)) {
            instructionsPerTab-=1;
        }
        if(IsKeyPressed(KEY_RIGHT)) {
            instructionsPerTab+=1;
        }
        if(IsKeyPressed(KEY_R)) {
            if(IsKeyDown(KEY_LEFT_SHIFT)) {
                resetCpu();
            } else {
                running=true;
            }
        }

        if( WindowShouldClose() ) {
            break;
        }

        // drawing
        BeginDrawing();
            ClearBackground(RAYWHITE);

            DrawText(TextFormat("[%d]", instructionsPerTab), 4, 4, 20, RED);

            Vector2 runningAnchor = {4, 30};
            if( running ) {
                DrawRectangleV(runningAnchor, (Vector2){60, 16}, ColorAlpha(LIME, 0.5));
                DrawText("RUNNING", runningAnchor.x+8, runningAnchor.y+3, 10, BLACK);
            } else {
                DrawRectangleV(runningAnchor, (Vector2){60, 16}, ColorAlpha(RED, 0.5));
                DrawText("STOPPED", runningAnchor.x+6, runningAnchor.y+3, 10, BLACK);
            }

            guiDrawCpuState();
            guiDrawMemView();
            guiDrawGraphics();

        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }

    // cleanup
    // unload our texture so it can be cleaned up
    UnloadTexture(wabbit);

    // destroy the window and cleanup the OpenGL context
    CloseWindow();

}

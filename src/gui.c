#include "controls.h"
#include "gb.h"
#include "graphics.h"
#include "raylib.h"
#include "gui.h"

Font firaFont;
uint16_t systemBreakpoint = 0xFFFF;

bool takeStep = false;
bool takeBigStep = false;
int bigStepCount = 0;

void guiDrawEmulatorControls(void)
{
    const Vector2 viewAnchor = {10, 845};  //570
    Vector2 anchor = viewAnchor;

    // Bounds
    GuiPanel((Rectangle){viewAnchor.x, viewAnchor.y, 480, 32}, NULL);
    anchor.x += 5;
    anchor.y += 5;

    // State
    if( running ) {
        DrawRectangleV(anchor, (Vector2){60, 22}, ColorAlpha(LIME, 0.5));
        DrawText("RUNNING", anchor.x+8, anchor.y+6, 10, BLACK);
    } else {
        DrawRectangleV(anchor, (Vector2){60, 22}, ColorAlpha(RED, 0.5));
        DrawText("STOPPED", anchor.x+6, anchor.y+6, 10, BLACK);
    }
    anchor.x += 65;

    // Step
    takeStep = GuiButton((Rectangle){anchor.x, anchor.y, 50, 22}, "STEP");
    anchor.x += 55;

    // Big Step
    takeBigStep = GuiButton((Rectangle){anchor.x, anchor.y, 50, 22}, "BGSTP");
    anchor.x += 55;

    // Big Step size
    static int bigStepSelected = 2;
    GuiToggleGroup((Rectangle){anchor.x, anchor.y+2, 30, 18}, "10;100;1K;10K;100K", &bigStepSelected);
    bigStepCount = pow(10,bigStepSelected+1);
    anchor.x += 34*5;

    // Run
    if( true == GuiButton((Rectangle){anchor.x, anchor.y, 50, 22}, "RUN")) {
        running = true;
    }
    anchor.x += 55;

    // Stop
    if( true == GuiButton((Rectangle){anchor.x, anchor.y, 50, 22}, "STOP")) {
        running = false;
    }
    anchor.x += 55;

    DrawFPS(1000, 0);
}

void guiInit(void)
{
    // Tell the window to use vsync and work on high DPI displays
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_RESIZABLE);

    // Create the window and OpenGL context
    InitWindow(1080 + 274, 900, "GameGirl");
    SetWindowMinSize(1080 + 274, 900);
    SetTargetFPS(60);

    // Load a texture from the resources directory
    //Texture wabbit = LoadTexture("Resources/wabbit_alpha.png");
    firaFont = LoadFontEx("resources/Fonts/FiraMono/FiraMonoNerdFont-Regular.otf", FONTSIZE, 0, 250);
}

void gui(void)
{

    int instructionsPerTab = 10000;
    bool keepRunning = true;

    // game loop
    // run the loop untill the user presses ESCAPE or presses the Close button on the window
    while (!WindowShouldClose() && keepRunning)
    {
        // Process keys
        takeStep = IsKeyPressed(KEY_SPACE);
        takeBigStep = IsKeyPressed(KEY_TAB);
        if(IsKeyPressed(KEY_R)) {
            if(IsKeyDown(KEY_LEFT_SHIFT)) {
                resetCpu();
            } else {
                running=true;
            }
        }

        ControlState controls;
        controls.buttonA = IsKeyDown(KEY_L);
        controls.buttonB = IsKeyDown(KEY_K);
        controls.select = IsKeyDown(KEY_V);
        controls.start = IsKeyDown(KEY_B);
        controls.dpadRight = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);
        controls.dpadLeft = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
        controls.dpadUp = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
        controls.dpadDown = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
        updateControls(controls);

        if( takeStep ) {
            executeInstruction(systemBreakpoint);
            running = false;
            takeStep = false;
        } else if( takeBigStep ) {
            for( int i=0; i<bigStepCount; i++) {
                executeInstruction(systemBreakpoint);
            }
            running = false;
            takeBigStep = false;
        } else if ( running ) {
            int maxInstructionsPerFrame = 20000;  // Theoretical max should be ~17500
            while( !guiUpdateScreen ) { //&& (0 < maxInstructionsPerFrame--) ) {
                if( executeInstruction(systemBreakpoint) ) {
                    running = false;
                    if( exitOnBreak ) {
                        keepRunning = false;
                    }
                    break;
                }
            }
        }

        // drawing
        BeginDrawing();
            ClearBackground(RAYWHITE);

            guiDrawGraphics();
            guiDrawControls();
            guiDrawCpuState();
            guiDrawMemView();
            guiDrawEmulatorControls();

        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }

    // cleanup
    // unload our texture so it can be cleaned up
    //UnloadTexture(wabbit);

    // destroy the window and cleanup the OpenGL context
    CloseWindow();

}

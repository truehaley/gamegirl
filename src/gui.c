#include "gb.h"
#include "raylib.h"
#include "gui.h"

Font firaFont;
uint16_t systemBreakpoint = 0xFFFF;

bool takeStep = false;
bool takeBigStep = false;
int bigStepCount = 0;

Vector2 guiDrawEmulatorControls(const Vector2 viewAnchor)
{
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

    return (Vector2){480, 32};
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

int gui(void)
{

    int instructionsPerTab = 10000;
    bool keepRunning = true;

    // game loop
    // run the loop untill the user presses ESCAPE or presses the Close button on the window
    while (!WindowShouldClose() && keepRunning)
    {
        // Process keys
        // Only change takeStep if the key is pressed so we don't miss gui button interations
        takeStep = (IsKeyPressed(KEY_SPACE))? true : takeStep;
        takeBigStep = (IsKeyPressed(KEY_TAB))? true : takeBigStep;
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
        controls.select = IsKeyDown(KEY_B);
        controls.start = IsKeyDown(KEY_N);
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

        Vector2 anchor, size;
        anchor = (Vector2){ GUI_PAD, GUI_PAD };
        // drawing
        BeginDrawing();
            ClearBackground(RAYWHITE);


            ////////////
            // Down the left side

            // Main Display
            Vector2 screenSize = size = guiDrawDisplayScreen(anchor);

            // Controls
            anchor.y += size.y + GUI_PAD;
            size = guiDrawControls(anchor);

            // CPU State
            anchor.y += size.y + GUI_PAD*2;
            size = guiDrawCpuState(anchor);

            // Emulator controls
            anchor.y += size.y + GUI_PAD;
            size = guiDrawEmulatorControls(anchor);

            ///////////
            // Down adjacent to the screen

            anchor = (Vector2){ anchor.x + screenSize.x + GUI_PAD, GUI_PAD };
            DrawFPS(anchor.x, anchor.y- (GUI_PAD/2));

            // Tile Maps
            anchor.y += 16;
            Vector2 tileMapSize = size = guiDrawDisplayTileMap(anchor, 0);
            size = guiDrawDisplayTileMap((Vector2){anchor.x + size.x + GUI_PAD, anchor.y}, 1);

            // Memory view
            anchor.y += size.y + GUI_PAD;
            size = guiDrawMemView(anchor);

            ///////////
            // Right side of the window

            // OAM Objects
            anchor = (Vector2){ anchor.x + 2*(tileMapSize.x + GUI_PAD), GUI_PAD };
            size = guiDrawDisplayObjects((Vector2){anchor.x + FONTWIDTH*2, anchor.y});

            // Tile Data
            anchor.y += size.y + GUI_PAD;
            Vector2 tileDataSize = size = guiDrawDisplayTileData(anchor);

        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }

    // cleanup

    // destroy the window and cleanup the OpenGL context
    CloseWindow();

    if( mooneye ) {
        return (mooneyeSuccess())? 42: 0x42;  // 42 success 66 fail
    } else {
        return 0;
    }
}

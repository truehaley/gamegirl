#include "raylib.h"
#include "raygui.h"
#include "gui.h"
#include "mem.h"
#include <stdlib.h>

Font firaFont;


// Draw and process scroll bar style edition controls
static void DrawStyleEditControls(void)
{
    // ScrollPanel style controls
    //----------------------------------------------------------
    GuiGroupBox((Rectangle){ 550, 170, 220, 205 }, "SCROLLBAR STYLE");

    int style = GuiGetStyle(SCROLLBAR, BORDER_WIDTH);
    GuiLabel((Rectangle){ 555, 195, 110, 10 }, "BORDER_WIDTH");
    GuiSpinner((Rectangle){ 670, 190, 90, 20 }, NULL, &style, 0, 6, false);
    GuiSetStyle(SCROLLBAR, BORDER_WIDTH, style);

    style = GuiGetStyle(SCROLLBAR, ARROWS_SIZE);
    GuiLabel((Rectangle){ 555, 220, 110, 10 }, "ARROWS_SIZE");
    GuiSpinner((Rectangle){ 670, 215, 90, 20 }, NULL, &style, 4, 14, false);
    GuiSetStyle(SCROLLBAR, ARROWS_SIZE, style);

    style = GuiGetStyle(SCROLLBAR, SLIDER_PADDING);
    GuiLabel((Rectangle){ 555, 245, 110, 10 }, "SLIDER_PADDING");
    GuiSpinner((Rectangle){ 670, 240, 90, 20 }, NULL, &style, 0, 14, false);
    GuiSetStyle(SCROLLBAR, SLIDER_PADDING, style);

    bool scrollBarArrows = GuiGetStyle(SCROLLBAR, ARROWS_VISIBLE);
    GuiCheckBox((Rectangle){ 565, 280, 20, 20 }, "ARROWS_VISIBLE", &scrollBarArrows);
    GuiSetStyle(SCROLLBAR, ARROWS_VISIBLE, scrollBarArrows);

    style = GuiGetStyle(SCROLLBAR, SLIDER_PADDING);
    GuiLabel((Rectangle){ 555, 325, 110, 10 }, "SLIDER_PADDING");
    GuiSpinner((Rectangle){ 670, 320, 90, 20 }, NULL, &style, 0, 14, false);
    GuiSetStyle(SCROLLBAR, SLIDER_PADDING, style);

    style = GuiGetStyle(SCROLLBAR, SLIDER_WIDTH);
    GuiLabel((Rectangle){ 555, 350, 110, 10 }, "SLIDER_WIDTH");
    GuiSpinner((Rectangle){ 670, 345, 90, 20 }, NULL, &style, 2, 100, false);
    GuiSetStyle(SCROLLBAR, SLIDER_WIDTH, style);

    const char *text = GuiGetStyle(LISTVIEW, SCROLLBAR_SIDE) == SCROLLBAR_LEFT_SIDE? "SCROLLBAR: LEFT" : "SCROLLBAR: RIGHT";
    bool toggleScrollBarSide = GuiGetStyle(LISTVIEW, SCROLLBAR_SIDE);
    GuiToggle((Rectangle){ 560, 110, 200, 35 }, text, &toggleScrollBarSide);
    GuiSetStyle(LISTVIEW, SCROLLBAR_SIDE, toggleScrollBarSide);
    //----------------------------------------------------------

    // ScrollBar style controls
    //----------------------------------------------------------
    GuiGroupBox((Rectangle){ 550, 20, 220, 135 }, "SCROLLPANEL STYLE");

    style = GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH);
    GuiLabel((Rectangle){ 555, 35, 110, 10 }, "SCROLLBAR_WIDTH");
    GuiSpinner((Rectangle){ 670, 30, 90, 20 }, NULL, &style, 6, 30, false);
    GuiSetStyle(LISTVIEW, SCROLLBAR_WIDTH, style);

    style = GuiGetStyle(DEFAULT, BORDER_WIDTH);
    GuiLabel((Rectangle){ 555, 60, 110, 10 }, "BORDER_WIDTH");
    GuiSpinner((Rectangle){ 670, 55, 90, 20 }, NULL, &style, 0, 20, false);
    GuiSetStyle(DEFAULT, BORDER_WIDTH, style);
    //----------------------------------------------------------
}




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

    Rectangle panelRec = { 20, 80, 200, 150 };
        Rectangle panelContentRec = {0, 0, 340, 340 };
        Rectangle panelView = { 0 };
        Vector2 panelScroll = { 99, -20 };

        Rectangle viewSelectRec = {20, 40, 50, 200};
        int activeView=0;
        bool showContentArea = true;


    // game loop
    while (!WindowShouldClose())// run the loop untill the user presses ESCAPE or presses the Close button on the window
    {
        // Update
        //----------------------------------------------------------------------------------
        // TODO: Implement required update logic
        //----------------------------------------------------------------------------------

        // drawing
        BeginDrawing();
            ClearBackground(RAYWHITE);

            //DrawText(TextFormat("[%f, %f]", panelScroll.x, panelScroll.y), 4, 4, 20, RED);

            guiDrawMemView();

            GuiScrollPanel(panelRec, NULL, panelContentRec, &panelScroll, &panelView);

            BeginScissorMode(panelView.x, panelView.y, panelView.width, panelView.height);
                GuiGrid((Rectangle){panelRec.x + panelScroll.x, panelRec.y + panelScroll.y, panelContentRec.width, panelContentRec.height}, NULL, 16, 3, NULL);
            EndScissorMode();

            if (showContentArea) DrawRectangle(panelRec.x + panelScroll.x, panelRec.y + panelScroll.y, panelContentRec.width, panelContentRec.height, Fade(RED, 0.1));

            //DrawStyleEditControls();

            GuiCheckBox((Rectangle){ 565, 80, 20, 20 }, "SHOW CONTENT AREA", &showContentArea);

            GuiSliderBar((Rectangle){ 590, 385, 145, 15}, "WIDTH", TextFormat("%i", (int)panelContentRec.width), &panelContentRec.width, 1, 600);
            GuiSliderBar((Rectangle){ 590, 410, 145, 15 }, "HEIGHT", TextFormat("%i", (int)panelContentRec.height), &panelContentRec.height, 1, 400);

            /*
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
            */
        // end the frame and get ready for the next one  (display frame, poll input, etc...)
        EndDrawing();
    }

    // cleanup
    // unload our texture so it can be cleaned up
    UnloadTexture(wabbit);

    // destroy the window and cleanup the OpenGL context
    CloseWindow();

}

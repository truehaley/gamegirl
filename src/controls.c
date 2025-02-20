#include "gb.h"
#include "gui.h"
#include "raylib.h"

void guiDrawControls(void)
{
    // Top left corner of the controls interface
    Vector2 viewAnchor = { 10, 144*3 + 20 };
    Vector2 anchor = viewAnchor;

    DrawRectangle(viewAnchor.x+30, viewAnchor.y+10 + 30, 90, 30, DARKGRAY);   // D-pad horiz
    DrawRectangle(viewAnchor.x+30 + 30, viewAnchor.y+10, 30, 90, DARKGRAY);   // D-pad vert
    DrawCircle(viewAnchor.x + 370, viewAnchor.y + 70, 22, MAROON);      // B
    DrawCircle(viewAnchor.x + 430, viewAnchor.y + 45, 22, MAROON);      // A
    DrawRectangle(viewAnchor.x + 170, viewAnchor.y + 100, 50, 15, LIGHTGRAY);    // Select
    DrawRectangle(viewAnchor.x + 250, viewAnchor.y + 100, 50, 15, LIGHTGRAY);    // Start
}

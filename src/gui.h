#ifndef __GUI_H__
#define __GUI_H__

#include "raylib.h"
#include "raygui.h"
#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ANCHOR_RECT(anchor, xloc, yloc, w, h)  ((Rectangle){ anchor.x+(xloc), anchor.y+(yloc), w, h })
#define FONTSIZE        (16.0f)
#define FONTWIDTH       (FONTSIZE/2)
#define GUI_PAD         (10.0f)

extern Font firaFont;
extern uint16_t systemBreakpoint;

void guiInit(void);
int gui(void);


#ifdef __cplusplus
}
#endif

#endif //__GUI_H__

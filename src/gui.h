#ifndef __GUI_H__
#define __GUI_H__

#include "raylib.h"
#include "raygui.h"
#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ANCHOR_RECT(anchor, xloc, yloc, w, h)  ((Rectangle){ anchor.x+(xloc), anchor.y+(yloc), w, h })
#define FONTSIZE        (16)
#define FONTWIDTH       (FONTSIZE/2)

extern Font firaFont;
extern uint16_t systemBreakpoint;


void gui(void);


#ifdef __cplusplus
}
#endif

#endif //__GUI_H__

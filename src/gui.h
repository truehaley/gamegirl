// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#ifndef __GUI_H__
#define __GUI_H__
// IWYU pragma: always_keep

// IWYU pragma: begin_exports
#include "raylib.h"
#include "raygui.h"
#include "gb_types.h"
// IWYU pragma: end_exports

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

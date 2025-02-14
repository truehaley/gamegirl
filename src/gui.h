#ifndef __GUI_H__
#define __GUI_H__

#ifdef __cplusplus
extern "C" {
#endif

#define ANCHOR_RECT(anchor, xloc, yloc, w, h)  ((Rectangle){ anchor.x+(xloc), anchor.y+(yloc), w, h })
#define FONTSIZE        (16)
#define FONTWIDTH       (FONTSIZE/2)

extern Font firaFont;

void gui(void);


#ifdef __cplusplus
}
#endif

#endif //__GUI_H__

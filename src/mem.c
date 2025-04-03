// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#include "gb.h"
#include "gui.h"

#define BYTES_PER_LINE  (16)
#define LINE_HEIGHT     (18)
#define PADDING         (4)
#define MAXVIEWS        (8)

typedef void (guiDrawMemLine)(Vector2 anchor, int view, int lineNum);

struct {
    memViewType type;
    union {
        RomImage *rom;
        RamImage *ram;
    };
    float lines;
    const char *name;
    uint16_t addrOffset;
    int highlight_offset;
    int highlight_length;
    Vector2 scrollPosition;
    guiDrawMemLine *lineDrawFunction;
} memView[MAXVIEWS];
static int numMemViews = 0;
static char memViewNames[128];

struct {
    const char *name;
    const RegViewList *view;
    Vector2 scrollPosition;
} regView[MAXVIEWS];
static int numRegViews = 0;
static char regViewNames[128];

static void updateMemViewNames(void)
{
    int offset=0;
    if(memView[0].type != NO_VIEW) {
        offset = offset + sprintf(memViewNames + offset, "%s", memView[0].name);
    }
    for(int view = 1; view < numMemViews; view++) {
        if(memView[view].type != NO_VIEW) {
            offset = offset + sprintf(memViewNames + offset, ";%s", memView[view].name);
        }
    }
}

static void updateRegViewNames(void)
{
    int offset=0;
    if(0 < numRegViews) {
        offset = offset + sprintf(regViewNames + offset, "%s", regView[0].name);
    }
    for(int view = 1; view < numRegViews; view++) {
        offset = offset + sprintf(regViewNames + offset, ";%s", regView[view].name);
    }
}


void setMemViewHighlight(int viewNum, int offset, int length)
{
    memView[viewNum].highlight_offset = offset;
    memView[viewNum].highlight_length = length;
}


static Color guiRomHighlightColor(const RomImage * const rom, int offset)
{
    switch(ROM_CONTENTTYPE(rom, offset)) {
        case ROM_CONTENT_INVALID:
            return ColorAlpha(RED, 0.3);
        case ROM_CONTENT_DATA:
            return ColorAlpha(GREEN, 0.3);
        case ROM_CONTENT_PREFIX:
            //return ColorAlpha(PURPLE, 0.4);
        case ROM_CONTENT_IMM8:
        case ROM_CONTENT_IMM16L:
        case ROM_CONTENT_IMM16H:
            //return ColorAlpha(PURPLE, 0.8);
        case ROM_CONTENT_OPCODE:
        case ROM_CONTENT_PREFIXOP:
            return ColorAlpha(PURPLE, 0.3);
        case ROM_CONTENT_UNKNOWN:
        default:
            return RAYWHITE;
    }
}


static void guiDrawRomLine(Vector2 anchor, int view, int lineNum)
{
    const RomImage * const rom = memView[view].rom;
    int offset = lineNum * BYTES_PER_LINE;
    if( offset >= memView[view].rom->size) {
        return;
    }
    const int maxOffset = MIN(offset + BYTES_PER_LINE, rom->size);
    anchor.y += 1;

    // line header
    DrawTextEx(firaFont, TextFormat("%04X |", offset + memView[view].addrOffset),  anchor, FONTSIZE, 0, BLACK);
    anchor.x += (FONTWIDTH*7);

    for(int index=0; offset < maxOffset; index++, offset++) {
        /* Disable memory type highlighting for now
        Color highlightColor = guiRomHighlightColor(rom, offset);
        Rectangle highlightRect = {anchor.x-FONTWIDTH/2, anchor.y-1, FONTWIDTH*3-1, LINE_HEIGHT-1};
        if ( ROM_HAS_MOREBYTES(rom, offset) && ((BYTES_PER_LINE/2-1) != index) && ((BYTES_PER_LINE-1) != index) ) {
            highlightRect.width += 1;
        }
        DrawRectangleRec(highlightRect, highlightColor);
        if( ROM_IS_CODE(rom, offset) ) {
            Color lineColor = ROM_IS_JUMPDEST(rom, offset)?GREEN:GRAY;
            if( (ROM_CONTENT_OPCODE == ROM_CONTENTTYPE(rom, offset))
                || ROM_CONTENT_PREFIX == ROM_CONTENTTYPE(rom, offset) ) {
                DrawLine(highlightRect.x+1,highlightRect.y,
                        highlightRect.x+1,highlightRect.y+highlightRect.height,
                        lineColor);
            }
            DrawLine(highlightRect.x, highlightRect.y+1,
                     highlightRect.x+highlightRect.width/2, highlightRect.y+1,
                     lineColor);
            DrawLine(highlightRect.x, highlightRect.y+highlightRect.height,
                     highlightRect.x+highlightRect.width/2, highlightRect.y+highlightRect.height,
                     lineColor);
            lineColor = (ROM_IS_ENDCODE(rom, offset))? RED: GRAY;
            DrawLine(highlightRect.x+highlightRect.width/2, highlightRect.y+1,
                     highlightRect.x+highlightRect.width, highlightRect.y+1,
                     lineColor);
            DrawLine(highlightRect.x+highlightRect.width/2, highlightRect.y+highlightRect.height,
                     highlightRect.x+highlightRect.width, highlightRect.y+highlightRect.height,
                     lineColor);
            if( ROM_HAS_MOREBYTES(rom, offset) ) {
                // when using highlightcolor again here, it has the really nice effect of just
                //  darkening the highlight a little bit since its drawing another line with alpha on top
                //  of existing highlightcolor
                lineColor = highlightColor;
            }
            DrawLine(highlightRect.x+highlightRect.width, highlightRect.y,
                    highlightRect.x+highlightRect.width, highlightRect.y+highlightRect.height,
                    lineColor);

        } */

        uint8_t data = rom->contents[offset];
        DrawTextEx(firaFont, TextFormat("%02X", data), anchor, FONTSIZE, 0, BLACK);
        anchor.x += FONTWIDTH*3;
        if( (BYTES_PER_LINE/2-1) == index ) {
            DrawTextEx(firaFont, ":",  anchor, FONTSIZE, 0, BLACK);
            anchor.x += FONTWIDTH*2;
        }
    }
}

static void guiDrawRamLine(Vector2 anchor, int view, int lineNum)
{
    const RamImage * const ram = memView[view].ram;
    int offset = lineNum * BYTES_PER_LINE;
    if( offset >= memView[view].rom->size) {
        return;
    }
    const int maxOffset = MIN(offset + BYTES_PER_LINE, ram->size);
    anchor.y += 1;

    // line header
    DrawTextEx(firaFont, TextFormat("%04X |", offset + memView[view].addrOffset),  anchor, FONTSIZE, 0, BLACK);
    anchor.x += (FONTWIDTH*7);

    for(int index=0; offset < maxOffset; index++, offset++) {
        uint8_t data = ram->contents[offset];
        DrawTextEx(firaFont, TextFormat("%02X", data), anchor, FONTSIZE, 0, BLACK);
        anchor.x += FONTWIDTH*3;
        if( (BYTES_PER_LINE/2-1) == index ) {
            DrawTextEx(firaFont, ":",  anchor, FONTSIZE, 0, BLACK);
            anchor.x += FONTWIDTH*2;
        }
    }
}

static void guiDrawCpuMemLine(Vector2 anchor, int view, int lineNum)
{
    int offset = lineNum * BYTES_PER_LINE;
    if( offset >= 65536) {
        return;
    }
    const int maxOffset = MIN(offset + BYTES_PER_LINE, 65536);
    anchor.y += 1;

    // line header
    DrawTextEx(firaFont, TextFormat("%04X |", offset),  anchor, FONTSIZE, 0, BLACK);
    anchor.x += (FONTWIDTH*7);

    for(int index=0; offset < maxOffset; index++, offset++) {
        uint8_t data = getMem8(offset);
        DrawTextEx(firaFont, TextFormat("%02X", data), anchor, FONTSIZE, 0, BLACK);
        anchor.x += FONTWIDTH*3;
        if( (BYTES_PER_LINE/2-1) == index ) {
            DrawTextEx(firaFont, ":",  anchor, FONTSIZE, 0, BLACK);
            anchor.x += FONTWIDTH*2;
        }
    }
}

void addRomView(RomImage *rom, const char * const name, uint16_t addrOffset)
{
    memView[numMemViews].type = ROM_VIEW;
    memView[numMemViews].rom = rom;
    memView[numMemViews].lines = (float)rom->size / BYTES_PER_LINE;
    memView[numMemViews].name = name;
    memView[numMemViews].addrOffset = addrOffset;
    memView[numMemViews].highlight_length = 0;
    memView[numMemViews].lineDrawFunction = guiDrawRomLine;
    numMemViews++;
    updateMemViewNames();
}

void addRamView(RamImage *ram, const char * const name, uint16_t addrOffset)
{
    memView[numMemViews].type = RAM_VIEW;
    memView[numMemViews].ram = ram;
    memView[numMemViews].lines = (float)ram->size / BYTES_PER_LINE;
    memView[numMemViews].name = name;
    memView[numMemViews].addrOffset = addrOffset;
    memView[numMemViews].highlight_length = 0;
    memView[numMemViews].lineDrawFunction = guiDrawRamLine;
    numMemViews++;
    updateMemViewNames();
}

void addRegView(const RegViewList *view, const char * const name)
{
    regView[numRegViews].name = name;
    regView[numRegViews].view = view;
    numRegViews++;
    updateRegViewNames();
}


Vector2 guiDrawMemView(const Vector2 viewAnchor)
{
    static int selectedView = 0;

    GuiToggleGroup(ANCHOR_RECT(viewAnchor, 0, 0, 50, 20), memViewNames, &selectedView);

    Rectangle contentSize = {
        0, 0,
        480-(float)GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)-2*GuiGetStyle(DEFAULT, BORDER_WIDTH),
        memView[selectedView].lines*LINE_HEIGHT+PADDING*2
    };

    Rectangle viewPort;
    GuiScrollPanel(ANCHOR_RECT(viewAnchor, 0, 24, 480, 16*LINE_HEIGHT+PADDING*2),
                    NULL, contentSize, &memView[selectedView].scrollPosition, &viewPort);

    int scrollY = floor(memView[selectedView].scrollPosition.y);
    int startLine = -(scrollY/LINE_HEIGHT);
    float scrollOffset = scrollY % LINE_HEIGHT;

    //DrawText(TextFormat("[%f, %d, %d]", memView[selectedView].scrollPosition.y, scrollY, startLine), 4, 4, 20, RED);

    BeginScissorMode(viewPort.x, viewPort.y, viewPort.width, viewPort.height);
        for( int viewRow = 0 ; viewRow < 16+1; viewRow++ ) {
            Vector2 lineAnchor = {viewAnchor.x+PADDING, viewPort.y+PADDING+scrollOffset+viewRow*LINE_HEIGHT};
            memView[selectedView].lineDrawFunction(lineAnchor, selectedView, startLine+viewRow);
        }
    EndScissorMode();

    return (Vector2){480, 16*LINE_HEIGHT+PADDING*2 + 24};
}

void dumpMemory(const uint8_t * const src, const int size)
{
    for(int offset = 0; offset < size; offset += 0x10) {
        printf("0x%04x | ", offset);
        for(int index = 0; index < 16; index++) {
            if( index == 8 ) { printf(": "); }
            printf("%02X ", src[offset+index]);
        }
        printf("\n");
    }
}

Status allocateRam(RamImage * const ram, const int size)
{
    memset(ram, 0, sizeof(RamImage));
    ram->contents = (uint8_t *)MemAlloc(size);
    if( NULL == ram->contents ) {
        return FAILURE;
    }
    ram->size = size;
    memset(ram->contents, 0x00, size);
    return SUCCESS;
}

void deallocateRam(RamImage * const ram)
{
    if( NULL != ram->contents ) {
        MemFree(ram->contents);
    }
    memset(ram, 0, sizeof(RamImage));
}

Status loadRom(RomImage * const rom, const char * const filename, int entrypoint)
{
    memset(rom, 0, sizeof(RomImage));

    rom->contents = LoadFileData(filename, &(rom->size));
    if( NULL == rom->contents ) {
        return FAILURE;
    }
    rom->contentFlags = (uint8_t *)MemAlloc(rom->size);
    // Assume contents are data to start
    memset(rom->contentFlags, ROM_CONTENT_DATA, rom->size);
    rom->entrypoint = entrypoint;
    ROM_SET_JUMPDEST(rom, entrypoint);
    return SUCCESS;
}

void unloadRom(RomImage * const rom)
{
    UnloadFileData(rom->contents);
    MemFree(rom->contentFlags);
    memset(rom, 0, sizeof(RomImage));
}



void memInit(void)
{
    memset(memView, 0, sizeof(memView));
    memset(memViewNames, 0, sizeof(memViewNames));
    memset(regView, 0, sizeof(regView));
    memset(regViewNames, 0, sizeof(regViewNames));

    // Add main cpu memory view
    memView[0].type = MEM_VIEW;
    memView[0].lines = (float)65536 / BYTES_PER_LINE;
    memView[0].name = "MEM";
    memView[0].highlight_length = 0;
    memView[0].lineDrawFunction = guiDrawCpuMemLine;
    numMemViews++;
    updateMemViewNames();
}


/*
//{ &regs..val, "", 0xFF4, {1, {{"", 8},}} },
//{ &regs..val, "", 0xFF4, {, {{"",1},{"",1},{"",1},{"",1},{"",1},{"",1},{"",1},}}},

const RegViewList displayRegView = {
    12,
    {
        { &regs.LCDC.val, "LCDC", "FF40", {8, {{"EN",1},{"wMAP",1},{"wEN",1},{"bTIL",1},{"bMAP",1},{"oSIZ",1},{"oEN",1},{"bwEN",1}}} },
        { &regs.STAT.val, "STAT", "FF41", {7, {{"RSVD",1},{"lycIE",1},{"oamIE",1},{"vblIE",1},{"hblIE",1},{"lyEQ",1},{"MODE",1},}}},
        { &regs.SCY.val,  "SCY",  "FF42", {1, {{"SCY", 8},}} },
        { &regs.SCX.val,  "SCX",  "FF43", {1, {{"SCX", 8},}} },
        { &regs.LY.val,   "LY",   "FF44", {1, {{"LY", 8},}} },
        { &regs.LYC.val,  "LYC",  "FF45", {1, {{"LYC", 8},}} },
        { &regs.OAM.val,  "OAM",  "FF46", {1, {{"OAM", 8},}} },
        { &regs.BGP.val,  "BGP",  "FF47", {4, {{"COL3",2},{"COL2",2},{"COL1",2},{"COL0",2},}}},
        { &regs.OBP0.val, "OBP0", "FF48", {4, {{"COL3",2},{"COL2",2},{"COL1",2},{"COL0",2},}}},
        { &regs.OBP1.val, "OBP1", "FF49", {4, {{"COL3",2},{"COL2",2},{"COL1",2},{"COL0",2},}}},
        { &regs.WY.val,   "WY",   "FF4A", {1, {{"WY", 8},}} },
        { &regs.WX.val,   "WX",   "FF4B", {1, {{"WX", 8},}} },
    }
};
*/

// Draws an individual field of a register
Vector2 guiDrawRegField(const Vector2 anchor, float minCharWidth, const char *label, const char *content)
{
    float labelWidth = MeasureText(label, 10);
    Vector2 contentSize = MeasureTextEx(firaFont, content, FONTSIZE, 0);
    float regWidth = MAX(FONTWIDTH*(minCharWidth+3), FONTWIDTH+MAX(labelWidth, contentSize.y));
    Rectangle bounds = (Rectangle){anchor.x, anchor.y, regWidth, FONTSIZE*1.8};
    Color color = GetColor(GuiGetStyle(DEFAULT, LINE_COLOR));

    // left, bottom, right
    DrawRectangle(bounds.x, bounds.y, 1, bounds.height, color);
    DrawRectangle(bounds.x, bounds.y + bounds.height - 1, bounds.width, 1, color);
    DrawRectangle(bounds.x + bounds.width - 1, bounds.y, 1, bounds.height, color);
    // Center the text
    DrawText(label, bounds.x + bounds.width/2 - labelWidth/2 ,bounds.y,10,color);
    DrawTextEx(firaFont, content, (Vector2){bounds.x + bounds.width/2 - contentSize.x/2, anchor.y+FONTSIZE*0.7f}, FONTSIZE, 0, BLACK);
    return (Vector2){bounds.width, bounds.height};
}

// Draws all the fields of a register
Vector2 guiDrawHexReg(const Vector2 viewAnchor, const RegViewFields regFields, int value)
{
    Vector2 anchor = viewAnchor;

    Vector2 size;
    int field = 0;
    int fieldPos = 8;
    while( field < regFields.count ) {
        fieldPos -= regFields.list[field].width;
        //             ( ( ( ( create mask of proper width ) << shift to field pos ) & extract ) >> shift home )
        int fieldVal = ( ( ( (0xFF >> (8-regFields.list[field].width)) << fieldPos ) & value) >> fieldPos );
        const char *content = (3 < regFields.list[field].width)?TextFormat("%02X", fieldVal):TextFormat("%01X", fieldVal);
        size = guiDrawRegField(anchor, 1, regFields.list[field].label, content);
        anchor.x += size.x;
        field++;
    }

    return (Vector2){anchor.x - viewAnchor.x, size.y};
}

// Draws a line representing a register's name, its offset, and all of its fields
Vector2 guiDrawHexRegLine(const Vector2 viewAnchor, RegView regv)
{
    Vector2 anchor = viewAnchor;
    Vector2 size;

    if( NULL == regv.value) {
        // Draw dividing text instead of register
        Color color = GetColor(GuiGetStyle(DEFAULT, LINE_COLOR));
        anchor.x += 10;
        DrawRectangle(anchor.x, viewAnchor.y+16, 50, 1, color);
        anchor.x += 50 + 10;
        DrawText(regv.name, anchor.x ,viewAnchor.y+12, 10, color);
        anchor.x += MeasureText(regv.name, 10) + 10;
        DrawRectangle(anchor.x, viewAnchor.y+16, 50, 1, color);
        anchor.x += 50;
    } else {
        // Draw normal register
        // line header
        anchor.x += 1;
        anchor.y += FONTSIZE*0.7f;
        DrawTextEx(firaFont, TextFormat("%s", regv.offset),  anchor, FONTSIZE, 0, BLACK);
        anchor.x += FONTWIDTH*6;
        DrawTextEx(firaFont, TextFormat("%5s", regv.name),  anchor, FONTSIZE, 0, BLACK);

        anchor.x += (FONTWIDTH*6);
        anchor.y = viewAnchor.y;
        size = guiDrawHexReg(anchor, regv.fields, *regv.value);

        anchor.x += size.x;
    }
    return (Vector2){viewAnchor.x - anchor.x, REGVIEW_DEFAULT_LINEHEIGHT};
}

Vector2 guiDrawRegView(const Vector2 viewAnchor)
{
    static int selectedView = 0;
    GuiToggleGroup(ANCHOR_RECT(viewAnchor, 0, 0, 50, 20), regViewNames, &selectedView);
    const RegViewList *currentView = regView[selectedView].view;

    Rectangle contentSize = {
        0, 0,
        480-(float)GuiGetStyle(LISTVIEW, SCROLLBAR_WIDTH)-2*GuiGetStyle(DEFAULT, BORDER_WIDTH),
        currentView->regCount*currentView->lineHeight +PADDING*2.0f
    };

    Rectangle viewPort;
    GuiScrollPanel(ANCHOR_RECT(viewAnchor, 0, 24, 480, 9*FONTSIZE*2 +PADDING*2),
                    NULL, contentSize, &regView[selectedView].scrollPosition, &viewPort);

    int scrollY = floor(regView[selectedView].scrollPosition.y);
    int startView = -(scrollY/currentView->lineHeight);
    float scrollOffset = scrollY % ((int)(currentView->lineHeight));

    BeginScissorMode(viewPort.x, viewPort.y, viewPort.width, viewPort.height);
        for( int viewRow = 0 ; viewRow < 16+1; viewRow++ ) {
            Vector2 lineAnchor = {viewAnchor.x+PADDING, viewPort.y+PADDING+scrollOffset+viewRow*currentView->lineHeight};
            if( startView + viewRow < currentView->regCount ) {
                if( NULL != currentView->guiDrawCustomRegLine ) {
                    currentView->guiDrawCustomRegLine(lineAnchor, startView+viewRow);
                } else {
                    RegView regv = currentView->regs[startView+viewRow];
                    guiDrawHexRegLine(lineAnchor, regv);
                }
            }
        }
    EndScissorMode();

    return (Vector2){480, 9*FONTSIZE*2+PADDING*2 + 24};
}

Vector2 guiDrawMemRegViews(const Vector2 viewAnchor)
{
    static int selectedView = 0;

    GuiToggleGroup(ANCHOR_RECT(viewAnchor, 0, 0, 100, 20), "MEMORY;REGISTERS", &selectedView);

    Vector2 anchor = { viewAnchor.x, viewAnchor.y+24 };
    Vector2 size;
    if(0 == selectedView) {
        size = guiDrawMemView(anchor);
    } else {
        size = guiDrawRegView(anchor);
    }

    return (Vector2){size.x, size.y+24};
}

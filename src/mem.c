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

static int numViews = 0;

static char memViewNames[128];


static void updateMemViewNames(void)
{
    int offset=0;
    if(memView[0].type != NO_VIEW) {
        offset = offset + sprintf(memViewNames + offset, "%s", memView[0].name);
    }
    for(int view = 1; view < numViews; view++) {
        if(memView[view].type != NO_VIEW) {
            offset = offset + sprintf(memViewNames + offset, ";%s", memView[view].name);
        }
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
        }

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
    memView[numViews].type = ROM_VIEW;
    memView[numViews].rom = rom;
    memView[numViews].lines = (float)rom->size / BYTES_PER_LINE;
    memView[numViews].name = name;
    memView[numViews].addrOffset = addrOffset;
    memView[numViews].highlight_length = 0;
    memView[numViews].lineDrawFunction = guiDrawRomLine;
    numViews++;
    updateMemViewNames();
}

void addRamView(RamImage *ram, const char * const name, uint16_t addrOffset)
{
    memView[numViews].type = RAM_VIEW;
    memView[numViews].ram = ram;
    memView[numViews].lines = (float)ram->size / BYTES_PER_LINE;
    memView[numViews].name = name;
    memView[numViews].addrOffset = addrOffset;
    memView[numViews].highlight_length = 0;
    memView[numViews].lineDrawFunction = guiDrawRamLine;
    numViews++;
    updateMemViewNames();
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

    // Add main cpu memory view
    memView[0].type = MEM_VIEW;
    memView[0].lines = (float)65536 / BYTES_PER_LINE;
    memView[0].name = "MEM";
    memView[0].highlight_length = 0;
    memView[0].lineDrawFunction = guiDrawCpuMemLine;
    numViews++;
    updateMemViewNames();
}

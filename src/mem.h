#ifndef __MEM_H__
#define __MEM_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ROM_CONTENTTYPE_MASK    (0x0F)
#define ROM_CONTENT_UNKNOWN     (0x00)
#define ROM_CONTENT_INVALID     (0x01)
#define ROM_CONTENT_DATA        (0x02)
#define ROM_CONTENT_OPCODE      (0x08)
#define ROM_CONTENT_IMM8        (ROM_CONTENT_OPCODE | 0x01)
#define ROM_CONTENT_IMM16L      (ROM_CONTENT_OPCODE | 0x02)
#define ROM_CONTENT_IMM16H      (ROM_CONTENT_OPCODE | 0x03)
#define ROM_CONTENT_PREFIX      (ROM_CONTENT_OPCODE | 0x04)
#define ROM_CONTENT_PREFIXOP    (ROM_CONTENT_OPCODE | 0x05)
#define ROM_MOREBYTES_MASK      (0x10)
#define ROM_ENDCODE_MASK        (0x40)
#define ROM_JUMPDEST_MASK       (0x80)

#define ROM_CONTENTTYPE(rom, offset)    (rom->contentFlags[(offset)] & ROM_CONTENTTYPE_MASK)
#define ROM_IS_DATA(rom, offset)        (ROM_CONTENTTYPE(rom, offset) == ROM_CONTENT_DATA)
#define ROM_IS_CODE(rom, offset)        ((ROM_CONTENTTYPE(rom, offset) & ROM_CONTENT_OPCODE) == ROM_CONTENT_OPCODE)
#define ROM_IS_INVALID(rom, offset)     (ROM_CONTENTTYPE(rom, offset) == ROM_CONTENT_INVALID)
#define ROM_IS_VALID(rom, offset)       (!ROM_IS_INVALID(rom, offset))
#define ROM_HAS_MOREBYTES(rom, offset)  ((rom->contentFlags[(offset)] & ROM_MOREBYTES_MASK) == ROM_MOREBYTES_MASK)
#define ROM_IS_ENDCODE(rom, offset)     ((rom->contentFlags[(offset)] & ROM_ENDCODE_MASK) == ROM_ENDCODE_MASK)
#define ROM_IS_JUMPDEST(rom, offset)    ((rom->contentFlags[(offset)] & ROM_JUMPDEST_MASK) == ROM_JUMPDEST_MASK)

#define ROM_SET_CONTENTTYPE(rom, offset, type)  do { rom->contentFlags[offset] = ((rom->contentFlags[offset] & (~ROM_CONTENTTYPE_MASK)) | type); } while(0)
#define ROM_SET_MOREBYTES(rom, offset)          do { rom->contentFlags[offset] = (rom->contentFlags[offset] | ROM_MOREBYTES_MASK); } while(0)
#define ROM_SET_ENDCODE(rom, offset)            do { rom->contentFlags[offset] = (rom->contentFlags[offset] | ROM_ENDCODE_MASK); } while(0)
#define ROM_SET_JUMPDEST(rom, offset)           do { rom->contentFlags[offset] = (rom->contentFlags[offset] | ROM_JUMPDEST_MASK); } while(0)

typedef enum {
    NO_VIEW = 0,
    ROM_VIEW,
    RAM_VIEW,
    REG_VIEW,
} memViewType;

void addRomView(RomImage *rom, const char * const name, uint16_t addrOffset);
void addRamView(RamImage *ram, const char * const name, uint16_t addrOffset);
void setMemViewHighlight(int viewNum, int offset, int length);
void guiDrawMemView(void);
void dumpMemory(const uint8_t * const src, const int size);


Status allocateRam(RamImage * const ram, const int size);
void deallocateRam(RamImage * const ram);

Status loadRom(RomImage * const rom, const char * const filename, int entrypoint);
void unloadRom(RomImage * const rom);


#ifdef __cplusplus
}
#endif

#endif //__MEM_H__

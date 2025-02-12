#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "utils.h"
#include "cpu.h"



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
    rom->size = 0;
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

void preprocessRom(RomImage * const rom, int offset)
{
    char buffer[32];
    int bytesPerInst;
    int destination=0;

    //printf("preprocess: %04x\n", offset);
    if( ROM_IS_CODE(rom, offset) || ROM_IS_INVALID(rom, offset) ) {
        return;  // we've already processed this destination
    }

    while( offset < rom-> size ) {
        bytesPerInst = disassembleInstruction(rom, offset, buffer, &destination);
        if( destination ) {
            // recurse to follow all jump destinations
            preprocessRom(rom, destination);
        }
        if( ROM_IS_ENDCODE(rom, offset) ) {
            break;
        }
        offset += bytesPerInst;
    }
}

void disassembleRom(RomImage * const rom)
{
    char buffer[32];
    int bytesPerInst;
    int destination;

    for(int offset = 0; offset < rom->size; ) {
        if( ROM_IS_DATA(rom, offset) ) {
            printf("DATA_%04X:", offset);
            int index = 0;
            while( ROM_IS_DATA(rom, offset+index) && ((offset+index) < rom->size) ) {
                if(0 == (index % 16) ) {
                    printf("\n        %04X | ", offset+index
                    );
                } else if( 8 == (index % 16) ) { printf(": "); }
                printf("%02X ", rom->contents[offset+index]);
                index++;
            }
            printf("\n");
            offset += index;
        } else if( ROM_IS_CODE(rom, offset) || ROM_IS_INVALID(rom, offset) ) {
            if( ROM_IS_JUMPDEST(rom, offset) ) {
                printf("LABEL_%04X:\n", offset);
            }
            printf("        %04X | ", offset);
            bytesPerInst = disassembleInstruction(rom, offset, buffer, &destination);
            for(int index = 0; index < 3; index++) {
                if (index < bytesPerInst) {
                    printf("%02X ", rom->contents[offset+index]);
                } else {
                    printf("   ");
                }
            }
            printf("|  %s\n", buffer);
            offset += bytesPerInst;
        } else {
            offset++;
        }
    }
}

const char * const aluDecode[] = {
    "ADD ",
    "ADC ",
    "SUB ",
    "SBC ",
    "AND ",
    "XOR ",
    "OR  ",
    "CP  ",
};

const char * const bitopDecode[] = {
    "RLCA",
    "RRCA",
    "RLA ",
    "RRA ",
    "DAA ",
    "CPL ",
    "SCF ",
    "CCF ",
};
const char * const pfx0Decode[] = {
    "RLC ",
    "RRC ",
    "RL  ",
    "RR  ",
    "SLA ",
    "SRA ",
    "SWAP",
    "SRL ",
};

const char * const r8Decode[] = {
    "B",
    "C",
    "D",
    "E",
    "H",
    "L",
    "[HL]",
    "A",
};

const char * const r16Decode[] = {
    "BC",
    "DE",
    "HL",
    "SP",
};

const char * const r16StkDecode[] = {
    "BC",
    "DE",
    "HL",
    "AF",
};

const char * const r16MemDecode[] = {
    "BC",
    "DE",
    "HL+",
    "HL-",
};

const char * const condDecode[] = {
    "NZ",
    "Z",
    "NC",
    "C",
};


// buffer parameter must be large enough!!!
// returns number of bytes consumed
int disassembleInstruction(RomImage * const rom, const int offset, char * const buffer, int *jumpDest)
{
    const uint8_t *memory = rom->contents;
    uint8_t instruction = memory[offset];
    char *buff = buffer;
    buff[0]='\0';
    // most instructions are only 1 byte
    int consumed = 1;
    // most instructions are valid
    bool valid = true;

    // Pull the most commonly referenced fields out of the instruction
    const uint8_t block = INST_BLOCK_EXTRACT(instruction);
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8_b  = INST_R8_B_EXTRACT(instruction);
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    const bool r16_flag = ((instruction & 0x08) == 0x08 );
    const uint8_t cond  = INST_COND_EXTRACT(instruction);
    const uint8_t op_a  = INST_R8_A_EXTRACT(instruction);
    const uint8_t op_b  = INST_R8_B_EXTRACT(instruction);

    uint16_t addr;

    switch( block ) {
    case INST_BLOCK0:
        switch( op_b ) {
        case 0:
            switch( op_a ) {
            case 0:
                buff = buff + sprintf(buff, "NOP");
                break;
            case 1:
                buff = buff + sprintf(buff, "LD   [0x%04X], SP", MEM_IMM16_EXTRACT(memory, offset));
                consumed = 3;
                break;
            case 2:
                buff = buff + sprintf(buff, "STOP");
                break;
            case 3:
                addr = MEM_IMM8_OFFSET_TO_ADDR(memory,offset);
                buff = buff + sprintf(buff, "JR   LABEL_%04X", addr);
                ROM_SET_JUMPDEST(rom, addr);
                *jumpDest = addr;
                ROM_SET_ENDCODE(rom, offset);
                consumed = 2;
                break;
            case 4:
            case 5:
            case 6:
            case 7:
                addr = MEM_IMM8_OFFSET_TO_ADDR(memory,offset);
                buff = buff + sprintf(buff, "JR   %s, LABEL_%04X", condDecode[cond], addr);
                ROM_SET_JUMPDEST(rom, addr);
                *jumpDest = addr;
                consumed = 2;
            }
            break;
        case 1:
            if( r16_flag ) {
                buff = buff + sprintf(buff, "ADD  HL, %s", r16Decode[r16]);
            } else {
                buff = buff + sprintf(buff, "LD   %s, 0x%04X", r16Decode[r16], MEM_IMM16_EXTRACT(memory, offset));
                consumed = 3;
            }
            break;
        case 2:
            if( r16_flag ) {
                buff = buff + sprintf(buff, "LD   A, [%s]", r16MemDecode[r16]);
            } else {
                buff = buff + sprintf(buff, "LD   [%s], A", r16MemDecode[r16]);
            }
            break;
        case 3:
            if( r16_flag ) {
                buff = buff + sprintf(buff, "DEC  %s", r16Decode[r16]);
            } else {
                buff = buff + sprintf(buff, "INC  %s", r16Decode[r16]);
            }
            break;
        case 4:
            buff = buff + sprintf(buff, "INC  %s", r8Decode[r8_a]);
            break;
        case 5:
            buff = buff + sprintf(buff, "DEC  %s", r8Decode[r8_a]);
            break;
        case 6:
            buff = buff + sprintf(buff, "LD   %s, 0x%02X", r8Decode[r8_a], memory[offset+1]);
            consumed = 2;
            break;
        case 7:
            buff = buff + sprintf(buff, "%s", bitopDecode[op_a]);
            break;
        }
        break;

    case INST_BLOCK1:
        // Mostly register to register loads
        if(INST_HALT == instruction) {
            buff = buff + sprintf(buff, "HALT");
        } else {
            buff = buff + sprintf(buff, "LD   %s, %s", r8Decode[r8_a], r8Decode[r8_b]);
        }
        break;

    case INST_BLOCK2:
        // all register based ALU operations
        buff = buff + sprintf(buff, "%s A, %s", aluDecode[op_a], r8Decode[r8_b]);
        break;

    case INST_BLOCK3:

        if(INST_PREFIX == instruction) {
            // Handle Prefix types
            instruction =  memory[offset+1];
            const uint8_t pfx_block = INST_BLOCK_EXTRACT(instruction);
            const uint8_t pfx_op_a  = INST_R8_A_EXTRACT(instruction);
            const uint8_t pfx_bit   = INST_R8_A_EXTRACT(instruction);
            const uint8_t pfx_r8    = INST_R8_B_EXTRACT(instruction);
            consumed = 2;
            switch( pfx_block ) {
            case INST_BLOCK0:
                buff = buff + sprintf(buff, "%s %s", pfx0Decode[pfx_op_a], r8Decode[pfx_r8]);
                break;
            case INST_BLOCK1:
                buff = buff + sprintf(buff, "BIT  %d, %s", pfx_bit, r8Decode[pfx_r8]);
                break;
            case INST_BLOCK2:
                buff = buff + sprintf(buff, "RES  %d, %s", pfx_bit, r8Decode[pfx_r8]);
                break;
            case INST_BLOCK3:
                buff = buff + sprintf(buff, "SET  %d, %s", pfx_bit, r8Decode[pfx_r8]);
                break;
            }

        } else {
            switch( op_b ) {
            case 0:
                switch( op_a ) {
                case 0:
                case 1:
                case 2:
                case 3:
                    buff = buff + sprintf(buff, "RET  %s", condDecode[cond]);
                    // not and endcode since return is conditional
                    break;
                case 4:
                    buff = buff + sprintf(buff, "LDH  [0x%04X], A", MEM_IMM8_H_TO_ADDR(memory, offset));
                    consumed = 2;
                    break;
                case 5:
                    // effectively: ADD SP, SP, E8
                    buff = buff + sprintf(buff, "ADD  SP, 0x%02X (signed)", memory[offset+1]);
                    consumed = 2;
                    break;
                case 6:
                    buff = buff + sprintf(buff, "LDH  A, [0x%04X]", MEM_IMM8_H_TO_ADDR(memory, offset));
                    consumed = 2;
                    break;
                case 7:
                    // effectively: ADD HL, SP, E8
                    buff = buff + sprintf(buff, "LD   HL, SP + 0x%02X (signed)", memory[offset+1]);
                    consumed = 2;
                    break;
                }
                break;
            case 1:
                switch( op_a ) {
                case 0:
                case 2:
                case 4:
                case 6:
                    buff = buff + sprintf(buff, "POP  %s", r16StkDecode[r16]);
                    break;
                case 1:
                    buff = buff + sprintf(buff, "RET");
                    ROM_SET_ENDCODE(rom, offset);
                    break;
                case 3:
                    buff = buff + sprintf(buff, "RETI");
                    ROM_SET_ENDCODE(rom, offset);
                    break;
                case 5:
                    buff = buff + sprintf(buff, "JP   HL");
                    // can't determine jump dest address unless running code
                    ROM_SET_ENDCODE(rom, offset);
                    break;
                case 7:
                    buff = buff + sprintf(buff, "LD   SP, HL");
                    break;
                }
                break;
            case 2:
                switch( op_a ) {
                case 0:
                case 1:
                case 2:
                case 3:
                    addr = MEM_IMM16_EXTRACT(memory, offset);
                    buff = buff + sprintf(buff, "JP   %s, LABEL_%04X", condDecode[cond], addr);
                    ROM_SET_JUMPDEST(rom, addr);
                    *jumpDest = addr;
                    consumed = 3;
                    break;
                case 4:
                    buff = buff + sprintf(buff, "LDH  [0xFF00+C], A");
                    break;
                case 6:
                    buff = buff + sprintf(buff, "LDH  A, [0xFF00+C]");
                    break;
                case 5:
                    buff = buff + sprintf(buff, "LD   [0x%04X], A", MEM_IMM16_EXTRACT(memory, offset));
                    consumed = 3;
                    break;
                case 7:
                    buff = buff + sprintf(buff, "LD   A, [0x%04X]", MEM_IMM16_EXTRACT(memory, offset));
                    consumed = 3;
                    break;
                }
                break;
            case 3:
                switch( op_a ) {
                case 0:
                    addr = MEM_IMM16_EXTRACT(memory, offset);
                    buff = buff + sprintf(buff, "JP   LABEL_%04X", addr);
                    ROM_SET_JUMPDEST(rom, addr);
                    *jumpDest = addr;
                    ROM_SET_ENDCODE(rom, offset);
                    consumed = 3;
                    break;
                case 1:  // PREFIX OPCODE HANDLED ABOVE, fall through to invalid here
                case 2:
                case 3:
                case 4:
                case 5:
                    buff = buff + sprintf(buff, "INVALID");
                    valid = false;
                    break;
                case 6:
                    buff = buff + sprintf(buff, "DI");
                    break;
                case 7:
                    buff = buff + sprintf(buff, "EI");
                    break;
                }
                break;
            case 4:
                if( op_a <= 3 ) {
                    addr = MEM_IMM16_EXTRACT(memory, offset);
                    buff = buff + sprintf(buff, "CALL %s, LABEL_%04X", condDecode[cond], addr);
                    ROM_SET_JUMPDEST(rom, addr);
                    *jumpDest = addr;
                    consumed = 3;
                } else {
                    buff = buff + sprintf(buff, "INVALID");
                    valid = false;
                }
                break;
            case 5:
                switch( op_a ) {
                case 0:
                case 2:
                case 4:
                case 6:
                    buff = buff + sprintf(buff, "PUSH %s", r16StkDecode[r16]);
                    break;
                case 1:
                    addr = MEM_IMM16_EXTRACT(memory, offset);
                    buff = buff + sprintf(buff, "CALL LABEL_%04X", addr);
                    ROM_SET_JUMPDEST(rom, addr);
                    *jumpDest = addr;
                    consumed = 3;
                    break;
                case 3:
                case 5:
                case 7:
                    buff = buff + sprintf(buff, "INVALID");
                    valid = false;
                    break;
                }
                break;
            case 6:
                // alu with imm8 parameter
                buff = buff + sprintf(buff, "%s A, 0x%02X", aluDecode[op_a], memory[offset+1]);
                consumed = 2;
                break;
            case 7:
                addr = INST_RST_TGT3_EXTRACT(instruction) * 8;
                buff = buff + sprintf(buff, "RST 0x%02X (LABEL_%04X)", addr, addr);
                ROM_SET_JUMPDEST(rom, addr);
                *jumpDest = addr;
                break;
            }
        }
        break;
    }

    if ( valid ) {
        ROM_SET_CONTENTTYPE(rom, offset, ROM_CONTENT_OPCODE);
        if( 2 == consumed ) {
            ROM_SET_CONTENTTYPE(rom, offset+1, ROM_CONTENT_IMM8);
        } else if ( 3 == consumed ) {
            ROM_SET_CONTENTTYPE(rom, offset+1, ROM_CONTENT_IMM16L);
            ROM_SET_CONTENTTYPE(rom, offset+2, ROM_CONTENT_IMM16H);
        }
    } else {
        ROM_SET_CONTENTTYPE(rom, offset, ROM_CONTENT_INVALID);
    }

    return consumed;
}

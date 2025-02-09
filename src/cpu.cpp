

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "utils.h"
#include "cpu.h"

int loadMemory(uint8_t *dest, const char *filename, const int maxsize)
{
    int dataSize = 0;
    uint8_t *data = LoadFileData(filename, &dataSize);

    dataSize = MIN(dataSize,maxsize);
    memcpy(dest, data, dataSize);

    UnloadFileData(data);
    return dataSize;
}

void dumpMemory(uint8_t *src, const int size)
{
    for(int offset = 0; offset < size; offset += 0x0F) {
        printf("0x%04x: ", offset);
        for(int index = 0; index < 16; index++) {
            printf("%02x ", src[offset+index]);
        }
        printf("\n");
    }
}

void disassemble(uint8_t *src, const int size)
{
    char buffer[32];
    int bytesPerInst;

    for(int offset = 0; offset < size; ) {
        printf("0x%04x: ", offset);
        bytesPerInst = disassembleInstruction(src, offset, buffer);
        for(int index = 0; index < 3; index++) {
            if (index < bytesPerInst) {
                printf("%02x ", src[offset+index]);
            } else {
                printf("   ");
            }
        }
        printf("|  %s\n", buffer);
        offset += bytesPerInst;
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
int disassembleInstruction(uint8_t *memory, const int offset, char *buffer)
{
    uint8_t instruction = memory[offset];
    char *buff = buffer;
    buff[0]='\0';
    // most instructions are only 1 byte
    int consumed = 1;

    switch( instruction & INST_BLOCK_MASK ) {
    case INST_BLOCK0:
        switch( instruction & 0x07) {
        case 0:
            if( 0x20 == (instruction & 0x20) ) {
                buff = buff + sprintf(buff, "JR   %s, 0x%04X", condDecode[INST_COND_EXTRACT(instruction)], MEM_IMM8_OFFSET_TO_ADDR(memory,offset));
                consumed = 2;
            } else {
                switch(INST_COND_EXTRACT(instruction)) {
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
                    buff = buff + sprintf(buff, "JR   0x%04X", MEM_IMM8_OFFSET_TO_ADDR(memory,offset));
                    consumed = 2;
                    break;
                }
            }
            break;
        case 1:
            if( 0x08 == (instruction & 0x08)) {
                buff = buff + sprintf(buff, "ADD   HL, %s", r16Decode[INST_R16_EXTRACT(instruction)]);
            } else {
                buff = buff + sprintf(buff, "LD   %s, 0x%04X", r16Decode[INST_R16_EXTRACT(instruction)], MEM_IMM16_EXTRACT(memory, offset));
                consumed = 3;
            }
            break;
        case 2:
            if( 0x08 == (instruction & 0x08)) {
                buff = buff + sprintf(buff, "LD   A, [%s]", r16MemDecode[INST_R16_EXTRACT(instruction)]);
            } else {
                buff = buff + sprintf(buff, "LD   [%s], A", r16MemDecode[INST_R16_EXTRACT(instruction)]);
            }
            break;
        case 3:
            if( 0x08 == (instruction & 0x08)) {
                buff = buff + sprintf(buff, "DEC  %s", r16Decode[INST_R16_EXTRACT(instruction)]);
            } else {
                buff = buff + sprintf(buff, "INC  %s", r16Decode[INST_R16_EXTRACT(instruction)]);
            }
            break;
        case 4:
            buff = buff + sprintf(buff, "INC  %s", r8Decode[INST_R8_A_EXTRACT(instruction)]);
            break;
        case 5:
            buff = buff + sprintf(buff, "DEC %s", r8Decode[INST_R8_A_EXTRACT(instruction)]);
            break;
        case 6:
            buff = buff + sprintf(buff, "LD   %s, 0x%02X", r8Decode[INST_R8_A_EXTRACT(instruction)], memory[offset+1]);
            consumed = 2;
            break;
        case 7:
            buff = buff + sprintf(buff, "%s", bitopDecode[INST_ALU_OP_EXTRACT(instruction)]);
            break;
        }
        break;

    case INST_BLOCK1:
        // Mostly register to register loads
        if(INST_HALT == instruction) {
            buff = buff + sprintf(buff, "HALT");
        } else {
            buff = buff + sprintf(buff, "LD   %s, %s", r8Decode[INST_LD_DST_EXTRACT(instruction)], r8Decode[INST_LD_SRC_EXTRACT(instruction)]);
        }
        break;

    case INST_BLOCK2:
        // all ALU operations
        buff = buff + sprintf(buff, "%s A, %s", aluDecode[INST_ALU_OP_EXTRACT(instruction)], r8Decode[INST_ALU_R8_EXTRACT(instruction)]);
        break;

    case INST_BLOCK3:

        if(INST_PREFIX == instruction) {
            // Handle Prefix types
            instruction =  memory[offset+1];
            consumed = 2;
            switch( instruction & INST_BLOCK_MASK ) {
            case INST_BLOCK0:
                buff = buff + sprintf(buff, "%s %s", pfx0Decode[INST_PFX0_OP_EXTRACT(instruction)], r8Decode[INST_PFX_R8_EXTRACT(instruction)]);
                break;
            case INST_BLOCK1:
                buff = buff + sprintf(buff, "BIT %d, %s", INST_PFX_BIT_EXTRACT(instruction), r8Decode[INST_PFX_R8_EXTRACT(instruction)]);
                break;
            case INST_BLOCK2:
                buff = buff + sprintf(buff, "RES %d, %s", INST_PFX_BIT_EXTRACT(instruction), r8Decode[INST_PFX_R8_EXTRACT(instruction)]);
                break;
            case INST_BLOCK3:
                buff = buff + sprintf(buff, "SET %d, %s", INST_PFX_BIT_EXTRACT(instruction), r8Decode[INST_PFX_R8_EXTRACT(instruction)]);
                break;
            }

        } else {
            switch( instruction & 0x07) {
            case 0:
                if( 0x20 == (instruction & 0x20)) {
                    switch( INST_COND_EXTRACT(instruction) ) {
                    case 0:
                        buff = buff + sprintf(buff, "LDH  [0x%04X], A", MEM_IMM8_H_TO_ADDR(memory, offset));
                        consumed = 2;
                        break;
                    case 1:
                        buff = buff + sprintf(buff, "ADD  SP, 0x%02X (signed)", memory[offset+1]);
                        consumed = 2;
                        break;
                    case 2:
                        buff = buff + sprintf(buff, "LDH  A, [0x%04X]", MEM_IMM8_H_TO_ADDR(memory, offset));
                        consumed = 2;
                        break;
                    case 3:
                        buff = buff + sprintf(buff, "LD   HL, SP + 0x%02X (signed)", memory[offset+1]);
                        consumed = 2;
                        break;
                    }
                } else {
                    buff = buff + sprintf(buff, "RET  %s", condDecode[INST_COND_EXTRACT(instruction)]);
                }
                break;
            case 1:
                if( 0x08 == (instruction & 0x08)) {
                    switch( (instruction & 0x30) >> 4 ) {
                    case 0:
                        buff = buff + sprintf(buff, "RET");
                        break;
                    case 1:
                        buff = buff + sprintf(buff, "RETI");
                        break;
                    case 2:
                        buff = buff + sprintf(buff, "JP   HL");
                        break;
                    case 3:
                        buff = buff + sprintf(buff, "LD   SP, HL");
                        break;
                    }
                } else {
                    buff = buff + sprintf(buff, "POP  %s", r16StkDecode[INST_R16_EXTRACT(instruction)]);
                }
                break;
            case 2:
                if( 0x20 == (instruction & 0x20)) {
                    switch( (instruction & 0x18) >> 3 ) {
                    case 0:
                        buff = buff + sprintf(buff, "LDH  [0xFF0C], A");
                        break;
                    case 1:
                        buff = buff + sprintf(buff, "LD   [0x%04X], A", MEM_IMM16_EXTRACT(memory, offset));
                        consumed = 3;
                        break;
                    case 2:
                        buff = buff + sprintf(buff, "LDH  A, [0xFF0C]");
                        break;
                    case 3:
                        buff = buff + sprintf(buff, "LD   A, [0x%04X]", MEM_IMM16_EXTRACT(memory, offset));
                        break;
                    }
                } else {
                    buff = buff + sprintf(buff, "JP   %s, 0x%04X", condDecode[INST_COND_EXTRACT(instruction)], MEM_IMM16_EXTRACT(memory, offset));
                    consumed = 3;
                }
                break;
            case 3:
                switch( (instruction & 0x38) >> 3 ) {
                case 0:
                    buff = buff + sprintf(buff, "JP   0x%04X", MEM_IMM16_EXTRACT(memory, offset));
                    consumed = 3;
                    break;
                case 6:
                    buff = buff + sprintf(buff, "DI");
                    break;
                case 7:
                    buff = buff + sprintf(buff, "EI");
                    break;
                default:
                    buff = buff + sprintf(buff, "INVALID");
                    break;
                }
                break;
            case 4:
                if( 0x20 == (instruction & 0x20)) {
                    buff = buff + sprintf(buff, "INVALID");
                } else {
                    buff = buff + sprintf(buff, "CALL %s, 0x%04X", condDecode[INST_COND_EXTRACT(instruction)], MEM_IMM16_EXTRACT(memory, offset));
                    consumed = 3;
                }
                break;
            case 5:
                if( 0x08 == (instruction & 0x08)) {
                    if( 0 == (instruction & 0x30) ) {
                        buff = buff + sprintf(buff, "CALL 0x%04X", MEM_IMM16_EXTRACT(memory, offset));
                        consumed = 3;
                    } else {
                        buff = buff + sprintf(buff, "INVALID");
                    }
                } else {
                    buff = buff + sprintf(buff, "PUSH %s", r16StkDecode[INST_R16_EXTRACT(instruction)]);
                }
                break;
            case 6:
                // alu with imm8 parameter
                buff = buff + sprintf(buff, "%s A, 0x%02X", aluDecode[INST_ALU_OP_EXTRACT(instruction)], memory[offset+1]);
                consumed = 2;
                break;
            case 7:
                buff = buff + sprintf(buff, "RST 0x%02X", INST_RST_TGT3_EXTRACT(instruction));
                break;
            }
        }
        break;
    }


    return consumed;
}

/*
switch( INST_COND_EXTRACT(instruction) ) {
case 0:
    break;
case 1:
    break;
case 2:
    break;
case 3:
    break;
}
*/

#include <stdio.h>
#include <string.h>
#include "raylib.h"
#include "utils.h"
#include "cpu.h"


void dumpMemory(uint8_t *src, const int size)
{
    for(int offset = 0; offset < size; offset += 0x10) {
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

    // Pull the most commonly referenced fields out of the instruction
    const uint8_t block = INST_BLOCK_EXTRACT(instruction);
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8_b  = INST_R8_B_EXTRACT(instruction);
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    const bool r16_flag = ((instruction & 0x08) == 0x08 );
    const uint8_t cond  = INST_COND_EXTRACT(instruction);
    const uint8_t op_a  = INST_R8_A_EXTRACT(instruction);
    const uint8_t op_b  = INST_R8_B_EXTRACT(instruction);

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
                buff = buff + sprintf(buff, "JR   0x%04X", MEM_IMM8_OFFSET_TO_ADDR(memory,offset));
                consumed = 2;
                break;
            case 4:
            case 5:
            case 6:
            case 7:
                buff = buff + sprintf(buff, "JR   %s, 0x%04X", condDecode[cond], MEM_IMM8_OFFSET_TO_ADDR(memory,offset));
                consumed = 2;
            }
            break;
        case 1:
            if( r16_flag ) {
                buff = buff + sprintf(buff, "ADD   HL, %s", r16Decode[r16]);
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
            buff = buff + sprintf(buff, "DEC %s", r8Decode[r8_a]);
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
                buff = buff + sprintf(buff, "BIT %d, %s", pfx_bit, r8Decode[pfx_r8]);
                break;
            case INST_BLOCK2:
                buff = buff + sprintf(buff, "RES %d, %s", pfx_bit, r8Decode[pfx_r8]);
                break;
            case INST_BLOCK3:
                buff = buff + sprintf(buff, "SET %d, %s", pfx_bit, r8Decode[pfx_r8]);
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
                    break;
                case 3:
                    buff = buff + sprintf(buff, "RETI");
                    break;
                case 5:
                    buff = buff + sprintf(buff, "JP   HL");
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
                    buff = buff + sprintf(buff, "JP   %s, 0x%04X", condDecode[cond], MEM_IMM16_EXTRACT(memory, offset));
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
                    buff = buff + sprintf(buff, "JP   0x%04X", MEM_IMM16_EXTRACT(memory, offset));
                    consumed = 3;
                    break;
                case 1:  // PREFIX OPCODE HANDLED ABOVE, fall through to invalid here
                case 2:
                case 3:
                case 4:
                case 5:
                    buff = buff + sprintf(buff, "INVALID");
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
                    buff = buff + sprintf(buff, "CALL %s, 0x%04X", condDecode[cond], MEM_IMM16_EXTRACT(memory, offset));
                    consumed = 3;
                } else {
                    buff = buff + sprintf(buff, "INVALID");
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
                    buff = buff + sprintf(buff, "CALL 0x%04X", MEM_IMM16_EXTRACT(memory, offset));
                    consumed = 3;
                    break;
                case 3:
                case 5:
                case 7:
                    buff = buff + sprintf(buff, "INVALID");
                    break;
                }
                break;
            case 6:
                // alu with imm8 parameter
                buff = buff + sprintf(buff, "%s A, 0x%02X", aluDecode[op_a], memory[offset+1]);
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

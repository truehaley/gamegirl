#ifndef __CPU_H__
#define __CPU_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int loadMemory(uint8_t *dest, const char *filename, const int maxsize);
void dumpMemory(uint8_t *src, const int size);
void disassemble(uint8_t *src, const int size);

int disassembleInstruction(uint8_t *memory, const int offset, char *buffer);





#define R8_B    (0x0)
#define R8_C    (0x1)
#define R8_D    (0x2)
#define R8_E    (0x3)
#define R8_H    (0x4)
#define R8_L    (0x5)
#define R8_HL   (0x6)
#define R8_A    (0x7)
#define R8_IMM8 (0x6)
#define R8_RST  (0x7)

#define R16_BC  (0x0)
#define R16_DE  (0x1)
#define R16_HL  (0x2)
#define R16_AF  (0x3)


#define INST_BLOCK_MASK (0xC0)
#define INST_BLOCK_SHFT (6)
#define INST_BLOCK_EXTRACT(instr)  ((instr & INST_BLOCK_MASK) >> INST_BLOCK_SHFT)
#define INST_BLOCK0     (0x00)
#define INST_BLOCK1     (0x01)
#define INST_BLOCK2     (0x02)
#define INST_BLOCK3     (0x03)

#define INST_PREFIX     (0xCB)
#define INST_HALT       (0x76)


#define INST_R8_A_MASK (0x38)
#define INST_R8_A_SHFT (3)
#define INST_R8_A_EXTRACT(instr)  ((instr & INST_R8_A_MASK) >> INST_R8_A_SHFT)
#define INST_R8_B_MASK (0x07)
#define INST_R8_B_SHFT (0)
#define INST_R8_B_EXTRACT(instr)  ((instr & INST_R8_B_MASK) >> INST_R8_B_SHFT)

#define INST_R16_MASK (0x30)
#define INST_R16_SHFT (4)
#define INST_R16_EXTRACT(instr)  ((instr & INST_R16_MASK) >> INST_R16_SHFT)


#define INST_OPER_MASK (0x38)
#define INST_OPER_SHFT (3)
#define INST_OPER_EXTRACT(instr)  ((instr & INST_OPER_MASK) >> INST_OPER_SHFT)
#define INST_ALU_OP_ADD  (0x0)
#define INST_ALU_OP_ADC  (0x1)
#define INST_ALU_OP_SUB  (0x2)
#define INST_ALU_OP_SBC  (0x3)
#define INST_ALU_OP_AND  (0x4)
#define INST_ALU_OP_XOR  (0x5)
#define INST_ALU_OP_OR   (0x6)
#define INST_ALU_OP_CP   (0x7)


#define INST_RST_TGT3_MASK    (0x38)
#define INST_RST_TGT3_SHFT    (3)
#define INST_RST_TGT3_EXTRACT(instr)  ((instr & INST_RST_TGT3_MASK) >> INST_RST_TGT3_SHFT)

#define INST_COND_MASK    (0x18)
#define INST_COND_SHFT    (3)
#define INST_COND_EXTRACT(instr)  ((instr & INST_COND_MASK) >> INST_COND_SHFT)

#define MEM_IMM16_EXTRACT(mem, offset)  ((mem[offset+2]<<8)|mem[offset+1])
#define MEM_IMM8_OFFSET_TO_ADDR(mem, offset)  (((int16_t)offset)+((int8_t)memory[offset+1])+2)
#define MEM_IMM8_H_TO_ADDR(mem, offset)  (0xFF00+memory[offset+1])

#ifdef __cplusplus
}
#endif
#endif

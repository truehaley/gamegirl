#ifndef __CPU_H__
#define __CPU_H__

#include "gb_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BOOTROM_ENTRY   (0)
#define CARTRIDGE_ENTRY (0x100)

#define R8_B    (0x0)
#define R8_C    (0x1)
#define R8_D    (0x2)
#define R8_E    (0x3)
#define R8_H    (0x4)
#define R8_L    (0x5)
#define R8_HL   (0x6)
#define R8_A    (0x7)
//#define R8_IMM8 (0x6)
//#define R8_RST  (0x7)

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
#define MEM_IMM8_OFFSET_TO_ADDR(mem, offset)  (((int16_t)offset)+((int8_t)mem[offset+1])+2)
#define MEM_IMM8_H_TO_ADDR(mem, offset)  (0xFF00+mem[offset+1])

typedef enum {
    INT_VBLANK = 0,
    INT_LCD    = 1,
    INT_TIMER  = 2,
    INT_SERIAL = 3,
    INT_JOYPAD = 4
} InterruptFlag;

void setIntReg8(uint16_t addr, uint8_t val8);
uint8_t getIntReg8(uint16_t addr);
void setIntFlag(InterruptFlag interrupt);

void guiDrawCpuState(void);
void resetCpu(void);
bool cpuStopped(void);
bool executeInstruction(const uint16_t breakpoint);

int disassemble2(char *buff, const uint8_t code[3], const int16_t addr);
int instructionSize(const uint8_t instruction);

void preprocessRom(RomImage * const rom, int offset);
void disassembleRom(RomImage * const rom);
int disassembleInstruction(RomImage * const rom, const int offset, char **buffer, int *jumpDest);

#ifdef __cplusplus
}
#endif
#endif // __CPU_H__

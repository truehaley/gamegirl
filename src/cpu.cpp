// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.
//
// Copyright (c) 2025 Haley Taylor (@truehaley)

#include "gb.h"
#include "gui.h"

static struct __attribute__((packed)) {
    union {
        uint16_t AF;
        struct {
            union {
                uint8_t F;
                struct {
                    uint8_t reserved:4;
                    uint8_t carry:1;
                    uint8_t halfCarry:1;
                    uint8_t sub:1;
                    uint8_t zero:1;
                } flags;
            };
            uint8_t A;
        };
    };
    union {
        uint16_t BC;
        struct {
            uint8_t C;
            uint8_t B;
        };
    };
    union {
        uint16_t DE;
        struct {
            uint8_t E;
            uint8_t D;
        };
    };
    union {
        uint16_t HL;
        struct {
            uint8_t L;
            uint8_t H;
        };
    };
    uint16_t SP;
    uint16_t PC;
} regs;

typedef union {
    uint8_t val;
    struct {
        uint8_t flags:5;
        uint8_t reserved:3;
    };
    struct {
        uint8_t vblank:1;
        uint8_t stat:1;
        uint8_t timer:1;
        uint8_t serial:1;
        uint8_t joypad:1;
        uint8_t :3;
    };
} InterruptRegister;

InterruptRegister ieReg = {0};
InterruptRegister ifReg = {0};

bool interruptsEnabled = false;
bool interruptsPendingEnable = false;
bool cpuHalted = false;

typedef struct {
    uint16_t addr;
    uint8_t code[3];
} InstructionDetail;
InstructionDetail instructionHistory[8];
int historyHead = 0;

static Instruction nextInstruction = {.val = 0};

void resetCpu(void)
{
    memset(&regs, 0, sizeof(regs));
    memset(instructionHistory, 0, sizeof(instructionHistory));
    ieReg.val = 0;
    ifReg.val = 0;
    interruptsEnabled = false;
    interruptsPendingEnable = false;
    cpuHalted = false;
    nextInstruction.val = getMem8(regs.PC++);
}

void setIntReg8(uint16_t addr, uint8_t val8)
{
    if( REG_IE_ADDR == addr ) {
        ieReg.val = val8;
    } else if( REG_IF_ADDR == addr ) {
        ifReg.val = val8;
    }
}

uint8_t getIntReg8(uint16_t addr)
{
    if( REG_IE_ADDR == addr ) {
        return ieReg.val;
    } else if( REG_IF_ADDR == addr ) {
        ifReg.reserved = 0x7;   // reads as 1s
        return ifReg.val;
    }
    return 0x00;
}

void setIntFlag(InterruptFlag interrupt)
{
    ifReg.flags |= (0x01 << interrupt);
    cpuHalted = false;
}

bool cpuStopped(void)
{
    // TODO
    return false;
}

static void guiDrawCpuReg8(Vector2 anchor, uint8_t val8, const char * const name)
{
    // header
    GuiGroupBox((Rectangle){anchor.x, anchor.y, FONTWIDTH*4, FONTSIZE*1.5}, name);
    //DrawTextEx(firaFont, name, anchor, FONTSIZE, 0, BLACK);
    anchor.x += FONTWIDTH;
    anchor.y += FONTSIZE/3;
    DrawTextEx(firaFont, TextFormat("%02X", val8),  anchor, FONTSIZE, 0, BLACK);
}

static void guiDrawCpuReg16(Vector2 anchor, uint16_t val16, const char * const name)
{
    // header
    GuiGroupBox((Rectangle){anchor.x, anchor.y, FONTWIDTH*8, FONTSIZE*1.5}, name);
    //DrawTextEx(firaFont, name, anchor, FONTSIZE, 0, BLACK);
    anchor.x += FONTWIDTH*2;
    anchor.y += FONTSIZE/3;
    DrawTextEx(firaFont, TextFormat("%04X", val16),  anchor, FONTSIZE, 0, BLACK);
}

int guiDisassembleDetail(char * const buffer, InstructionDetail *id)
{
    char * buff = buffer;

    buff = buff + sprintf(buff, "%04X | ", id->addr);
    int bytesPerInst = instructionSize(id->code[0]);
    for(int index = 0; index < 3; index++) {
        if (index < bytesPerInst) {
            buff = buff + sprintf(buff, "%02X ", id->code[index]);
        } else {
            buff = buff + sprintf(buff,"   ");
        }
    }
    buff = buff + sprintf(buff, "|  ");
    buff = buff + disassemble2(buff, id->code, id->addr);
    buff = buff + sprintf(buff, "\n");
    return buff - buffer;
}

Vector2 guiDrawCpuState(const Vector2 viewAnchor)
{
    // Top left corner of the cpu display

    Vector2 regAnchor1 = { viewAnchor.x, viewAnchor.y };
    Vector2 regAnchor2 = { regAnchor1.x + FONTWIDTH*4, regAnchor1.y };
    guiDrawCpuReg8(regAnchor1, regs.A, "A");
    guiDrawCpuReg8(regAnchor2, regs.F, "F");
    regAnchor1.y += FONTSIZE*2;
    regAnchor2.y += FONTSIZE*2;
    guiDrawCpuReg8(regAnchor1, regs.B, "B");
    guiDrawCpuReg8(regAnchor2, regs.C, "C");
    regAnchor1.y += FONTSIZE*2;
    regAnchor2.y += FONTSIZE*2;
    guiDrawCpuReg8(regAnchor1, regs.D, "D");
    guiDrawCpuReg8(regAnchor2, regs.E, "E");
    regAnchor1.y += FONTSIZE*2;
    regAnchor2.y += FONTSIZE*2;
    guiDrawCpuReg8(regAnchor1, regs.H, "H");
    guiDrawCpuReg8(regAnchor2, regs.L, "L");
    regAnchor1.y += FONTSIZE*2;
    guiDrawCpuReg16(regAnchor1, regs.SP, "SP");
    regAnchor1.y += FONTSIZE*2;
    guiDrawCpuReg16(regAnchor1, regs.PC, "PC");
    regAnchor1.y += FONTSIZE*2;
    uint16_t flags = regs.flags.zero << 12 | regs.flags.sub << 8 | regs.flags.halfCarry << 4 | regs.flags.carry << 0;
    guiDrawCpuReg16(regAnchor1, flags, "Z N H C");
    regAnchor1.y += FONTSIZE*2;

    static char buffer[2048];
    char *buff = buffer;
    int nextPC = regs.PC-1;
    int lines=0;

    // Instruction history
    int historyOffset = (historyHead + 8 - 7) & 0x7;
    do {
        buff = buff + guiDisassembleDetail(buff, &instructionHistory[historyOffset]);
        historyOffset = (historyOffset + 1) & 0x7;
    } while( ++lines < 7 );
    buff = buff + sprintf(buff, "\n");

    // Upcoming instructions
    do {
        InstructionDetail id;
        id.addr = nextPC;
        id.code[0] = getMem8(nextPC);
        id.code[1] = getMem8(nextPC+1);
        id.code[2] = getMem8(nextPC+2);
        buff = buff + guiDisassembleDetail(buff, &id);
        nextPC += instructionSize(id.code[0]);
    } while( ++lines < 11 );

    // Background highlight of the next instruction to be run
    DrawRectangle(viewAnchor.x+90, viewAnchor.y+FONTSIZE*9, 350, FONTSIZE, ColorAlpha(GOLD, 0.3));
    // Disssassembly view
    DrawTextEx(firaFont, buffer, (Vector2){viewAnchor.x + 95, viewAnchor.y}, FONTSIZE, 0, BLACK);

    return {90+350, regAnchor1.y-viewAnchor.y};
}


static bool checkCond(const Instruction instruction)
{
    switch(instruction.cond) {
        case 0: // NZ
            return (regs.flags.zero == 0);
        case 1: // Z
            return (regs.flags.zero == 1);
        case 2: // NC
            return (regs.flags.carry == 0);
        case 3: // C
            return (regs.flags.carry == 1);
        default: // impossible, squelch warning
            return 0;
    }
}

static uint8_t * const r8_regs[8] = {
    &regs.B,
    &regs.C,
    &regs.D,
    &regs.E,
    &regs.H,
    &regs.L,
    0,
    &regs.A
};

static uint8_t getReg8(uint8_t r8)
{
    if( r8 == R8_HL ) {
        return readMem8(regs.HL);
    } else {
        return *r8_regs[r8];
    }
}

static void setReg8(uint8_t r8, uint8_t val8)
{
    if( r8 == R8_HL ) {
        writeMem8(regs.HL, val8);
    } else {
        *r8_regs[r8] = val8;
    }
}

static uint16_t getReg16(uint8_t r16)
{
    switch(r16) {
        case 0:
            return regs.BC;
        case 1:
            return regs.DE;
        case 2:
            return regs.HL;
        case 3:
            return regs.SP;
        default: // not possible if called correctly, squelch warning
            return 0;
    }
}

static void setReg16(uint8_t r16, uint16_t val16)
{
    switch(r16) {
        case 0:
            regs.BC = val16;
            break;
        case 1:
            regs.DE = val16;
            break;
        case 2:
            regs.HL = val16;
            break;
        case 3:
            regs.SP = val16;
            break;
    }
}

static uint8_t readMem8R16(uint8_t r16)
{
    switch(r16) {
        case 0:
            return readMem8(regs.BC);
        case 1:
            return readMem8(regs.DE);
        case 2:
            return readMem8(regs.HL++);
        case 3:
            return readMem8(regs.HL--);
        default: // not possible if called correctly, squelch warning
            return 0;
    }
}

static void writeMem8R16(uint8_t r16, uint16_t val8)
{
    switch(r16) {
        case 0:
            writeMem8(regs.BC, val8);
            break;
        case 1:
            writeMem8(regs.DE, val8);
            break;
        case 2:
            writeMem8(regs.HL++, val8);
            break;
        case 3:
            writeMem8(regs.HL--, val8);
            break;
    }
}

static uint8_t readImm8(void)
{
    return readMem8(regs.PC++);
}

static uint16_t readImm16(void)
{
    uint16_t val16 = readMem16(regs.PC);
    regs.PC += 2;
    return val16;
}

static uint16_t __inline pop16(void)
{
    uint16_t val16 = readMem8(regs.SP++);
    val16 |= readMem8(regs.SP++) << 8;
    return val16;
}

static void __inline push16(uint16_t val16)
{
    cpuCycle(); // pre-decrement takes a cycle
    writeMem8(--regs.SP, MSB(val16));
    writeMem8(--regs.SP, LSB(val16));
}


static bool nop(const Instruction instruction)
{
    // Nothing to see here
    // no flags affected
    return false;
}

static bool ld_ma16_sp(const Instruction instruction)
{
    // LD [imm16], SP
    uint16_t addr = readImm16();
    writeMem16(addr, regs.SP);
    // no flags affected
    return false;
}

static bool stop(const Instruction instruction)
{
    // TODO
    return false;
}

static bool jr_e8(const Instruction instruction)
{
    // JR imm8
    const uint16_t instAddr = regs.PC-1;
    int8_t offset = (int8_t)readImm8();
    cpuCycle();   // ALU op to add the offset
    regs.PC = offset + regs.PC;
    // no flags affected
    return (instAddr == regs.PC);
}

static bool jr_cc_e8(const Instruction instruction)
{
    // JR cc, imm8
    const uint16_t instAddr = regs.PC-1;
    int8_t offset = (int8_t)readImm8();
    if( checkCond(instruction) ) {
        cpuCycle();   // ALU op to add the offset
        regs.PC = offset + regs.PC;
    }
    // no flags affected
    return (instAddr == regs.PC);
}

static bool ld_r16_i16(const Instruction instruction)
{
    // LD r16, imm16
    setReg16(instruction.r16, readImm16());
    // no flags affected
    return false;
}

static bool add_hl_r16(const Instruction instruction)
{
    // ADD HL, r16
    uint32_t val16 = getReg16(instruction.r16);
    uint32_t result = val16 + regs.HL;
    uint16_t halfResult = (val16 & 0x0FFF) + (regs.HL & 0x0FFF);
    regs.HL = (uint16_t)result;
    // Takes an extra cycle since this is executed as 2 separate 8 bit adds
    cpuCycle();
    // zero flag not affected
    regs.flags.sub = 0;
    regs.flags.halfCarry = (0x1000&halfResult)>>12;
    regs.flags.carry = (0x00010000&result)>>16;
    return false;
}

static bool ld_mr16_a(const Instruction instruction)
{
    // LD [r16mem], A
    writeMem8R16(instruction.r16, regs.A);
    // no flags affected
    return false;
}

static bool ld_a_mr16(const Instruction instruction)
{
    // LD A, [r16mem]
    regs.A = readMem8R16(instruction.r16);
    // no flags affected
    return false;
}

static bool inc_r16(const Instruction instruction)
{
    // INC r16
    setReg16(instruction.r16, getReg16(instruction.r16)+1);
    // Takes an extra cycle because the IDU is used for the increment/decrement
    cpuCycle();
    // no flags affected
    return false;
}

static bool dec_r16(const Instruction instruction)
{
    // DEC
    setReg16(instruction.r16, getReg16(instruction.r16)-1);
    // Takes an extra cycle because the IDU is used for the increment/decrement
    cpuCycle();
    // no flags affected
    return false;
}

static bool inc_r8(const Instruction instruction)
{
    // INC r8
    uint8_t v8 = getReg8(instruction.r8_dst);
    uint8_t result= v8+1;
    uint8_t halfResult = (v8&0xF)+1;
    setReg8(instruction.r8_dst, result);
    regs.flags.zero = (0==result)?1:0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = (0x0010&halfResult)>>4;
    // carry not affected
    return false;
}

static bool dec_r8(const Instruction instruction)
{
    // DEC r8
    uint8_t v8 = getReg8(instruction.r8_dst);
    uint8_t result= v8-1;
    uint8_t halfResult = (v8&0xF)-1;
    setReg8(instruction.r8_dst, result);
    regs.flags.zero = (0==result)?1:0;
    regs.flags.sub = 1;
    regs.flags.halfCarry = (0x0010&halfResult)>>4;
    // carry not affected
    return false;
}

static bool ld_r8_i8(const Instruction instruction)
{
    // LD r8, imm8
    setReg8(instruction.r8_dst,readImm8());
    // no flags affected
    return false;
}

static bool rlc_r8(const Instruction instruction)
{
    // RLC r8
    uint8_t val8 = getReg8(instruction.r8_op);
    uint8_t result8 = (val8 << 1) | ((val8 & 0x80) >> 7);
    setReg8(instruction.r8_op, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = ((val8 & 0x80) >> 7);
    return false;
}

static bool rlc_a(const Instruction instruction)
{
    // RLCA
    // this version of always operates on reg A and leaves zero flag clear
    rlc_r8(instruction);
    regs.flags.zero = 0;
    // other flags adjusted above
    return false;
}

static bool rl_r8(const Instruction instruction)
{
    // RL r8
    uint8_t val8 = getReg8(instruction.r8_op);
    uint8_t result8 = (val8 << 1) | regs.flags.carry;
    setReg8(instruction.r8_op, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x80) >> 7;
    return false;
}

static bool rl_a(const Instruction instruction)
{
    // RLA
    // this version of always operates on reg A and leaves zero flag clear
    rl_r8(instruction);
    regs.flags.zero = 0;
    // other flags adjusted above
    return false;
}

static bool rrc_r8(const Instruction instruction)
{
    // RRC r8
    uint8_t val8 = getReg8(instruction.r8_op);
    uint8_t result8 = ((val8 & 0x01) << 7) | (val8 >> 1);
    setReg8(instruction.r8_op, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x01);
    return false;
}

static bool rrc_a(const Instruction instruction)
{
    // RRCA
    // this version of always operates on reg A and leaves zero flag clear
    rrc_r8(instruction);
    regs.flags.zero = 0;
    // other flags adjusted above
    return false;
}

static bool rr_r8(const Instruction instruction)
{
    // RR r8
    uint8_t val8 = getReg8(instruction.r8_op);
    uint8_t result8 = (regs.flags.carry << 7) | (val8 >> 1);
    setReg8(instruction.r8_op, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x01);
    return false;
}

static bool rr_a(const Instruction instruction)
{
    // RRA
    // this version of always operates on reg A and leaves zero flag clear
    rr_r8(instruction);
    regs.flags.zero = 0;
    // other flags adjusted above
    return false;
}

static bool daa(const Instruction instruction)
{
    // DAA
    uint8_t adj;
    if(regs.flags.sub) {
        adj = (regs.flags.halfCarry)? 0x06 : 0x00;
        adj += (regs.flags.carry)? 0x60 : 0x00;
        regs.A -= adj;
    } else {
        adj = ((regs.flags.halfCarry) || ((regs.A&0x0F) > 0x09))? 0x06 : 0x00;
        if( (regs.flags.carry) || (regs.A>0x99) ) {
            adj += 0x60;
            regs.A += adj;
            regs.flags.carry = 1;
        } else {
            regs.A += adj;
        }
    }
    regs.flags.zero = (0 == regs.A)?1:0;
    // sub flag not affected
    regs.flags.halfCarry = 0;
    // carry set in 1 case above, otherwise unchanged
    return false;
}

static bool cpl(const Instruction instruction)
{
    // CPL
    // compliment A
    regs.A = ~regs.A;
    // zero flag not affected
    regs.flags.sub = 1;
    regs.flags.halfCarry = 1;
    // carry flag not affected
    return false;
}

static bool scf(const Instruction instruction)
{
    // SCF
    // set carry flag
    // zero flag not affected
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = 1;
    return false;
}

static bool ccf(const Instruction instruction)
{
    // CCF
    // compliment carry flag
    // zero flag not affected
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = ~regs.flags.carry;
    return false;
}

static __inline void alu(uint8_t op, uint8_t val8)
{
    uint8_t A = regs.A;
    uint16_t result;
    uint8_t halfResult = 0;
    regs.flags.sub = 0;
    switch( op ) {
        case 0: // ADD
            result= A+val8;
            halfResult = (A&0xF)+(val8&0xF);
            break;
        case 1: // ADC
            result= A+val8+regs.flags.carry;
            halfResult = (A&0xF)+(val8&0xF)+regs.flags.carry;
            break;
        case 2: // SUB
            regs.flags.sub = 1;
            result= A-val8;
            halfResult = (A&0xF)-(val8&0xF);
            break;
        case 3: // SBC
            regs.flags.sub = 1;
            result= A-val8-regs.flags.carry;
            halfResult = (A&0xF)-(val8&0xF)-regs.flags.carry;
            break;
        case 4: // AND
            result= A&val8;
            halfResult = 0x10;
            break;
        case 5: // XOR
            result= A^val8;
            halfResult = 0;
            break;
        case 6: // OR
            result= A|val8;
            halfResult = 0;
            break;
        case 7: // CP
            result= A-val8;
            regs.flags.sub = 1;
            halfResult = (A&0xF)-(val8&0xF);
            break;
    }
    if( 7 != op ) {
        regs.A = result;
    }
    regs.flags.zero = (0==(uint8_t)result)?1:0;
    // sub flag adjusted above
    regs.flags.carry = (0x0100&result)>>8;
    regs.flags.halfCarry = (0x10&halfResult)>>4;
}

static bool alu_r8(const Instruction instruction)
{
    // ALU A, r8
    uint8_t val8 = getReg8(instruction.r8_op);
    alu(instruction.op, val8);
    // flags adjusted in alu helper
    return false;
}

static bool halt(const Instruction instruction)
{
    // TODO
    if( 0 == (ifReg.flags & ieReg.flags) ) {
        // only halt if nothing is pending
        cpuHalted = true;
    }
    return false;
}

static bool ld_r8_r8(const Instruction instruction)
{
    setReg8(instruction.r8_dst,getReg8(instruction.r8_src));
    // no flags affected
    return false; // ((R8_B == r8_dst) && (R8_B == r8_src));
}

static bool pop_r16(const Instruction instruction)
{
    // POP r16
    uint16_t val16 = pop16();
    switch(instruction.r16) {
        case 0:
            regs.BC = val16;
            break;
        case 1:
            regs.DE = val16;
            break;
        case 2:
            regs.HL = val16;
            break;
        case 3:
            regs.AF = (val16 & 0xFFF0);
            break;
    }
    // AF affects flags, rest do not
    return false;
}

static bool push_r16(const Instruction instruction)
{
    // PUSH r16
    switch(instruction.r16) {
        case 0:
            push16(regs.BC);
            break;
        case 1:
            push16(regs.DE);
            break;
        case 2:
            push16(regs.HL);
            break;
        case 3:
            push16(regs.AF);
            break;
    }
    // No flags affected
    return false;
}

static bool jp_i16(const Instruction instruction)
{
    // JP imm16
    const uint16_t instAddr = regs.PC-1;
    uint16_t pc = readImm16();
    cpuCycle();   // extra cycle to transfer to PC
    regs.PC = pc;
    // no flags affected
    return (instAddr == regs.PC);
}

static bool jp_cc_i16(const Instruction instruction)
{
    // JP cond, imm16
    const uint16_t instAddr = regs.PC-1;
    uint16_t pc = readImm16();
    if( checkCond(instruction) ) {
        cpuCycle();   // extra cycle to transfer to PC
        regs.PC = pc;
    }
    // no flags affected
    return (instAddr == regs.PC);
}

static bool jp_hl(const Instruction instruction)
{
    // JP HL
    const uint16_t instAddr = regs.PC-1;
    regs.PC = regs.HL;
    // no flags affected
    return (instAddr == regs.PC);
}

static bool call_i16(const Instruction instruction)
{
    // CALL imm16
    const uint16_t instAddr = regs.PC-1;
    uint16_t pc = readImm16();
    push16(regs.PC);
    regs.PC = pc;
    // no flags affected
    return (instAddr == regs.PC);
}

static bool call_cc_i16(const Instruction instruction)
{
    // CALL cond, imm16
    const uint16_t instAddr = regs.PC-1;
    uint16_t pc = readImm16();
    if( checkCond(instruction) ) {
        push16(regs.PC);
        regs.PC = pc;
    }
    // no flags affected
    return (instAddr == regs.PC);
}

static bool rst(const Instruction instruction)
{
    // RST vec
    const uint16_t instAddr = regs.PC-1;
    push16(regs.PC);
    regs.PC = instruction.tgt * 8;
    // no flags affected
    return (instAddr == regs.PC);
}

static bool ret(const Instruction instruction)
{
    // RET
    uint16_t pc = pop16();
    cpuCycle();  // extra cycle to move to PC
    regs.PC = pc;
    // no flags affected
    return false;
}

static bool ret_cc(const Instruction instruction)
{
    // RET cond
    cpuCycle(); // takes a cycle to test conditions
    if( checkCond(instruction) ) {
        ret(instruction);
    }
    // no flags affected
    return false;
}

static bool reti(const Instruction instruction)
{
    // RETI
    ret(instruction);
    interruptsPendingEnable = true;
    interruptsEnabled = true;
    // no flags affected
    return false;
}

static bool alu_i8(const Instruction instruction)
{
    // ALU A, imm8
    uint8_t val8 = readImm8();
    alu(instruction.op, val8);
    // flags adjusted in ALU helper
    return false;
}

static bool invalid(const Instruction instruction)
{
    // INVALID INSTRUCTION
    // TODO
    // no flags affected
    return true;
}

static bool ldh_mc_a(const Instruction instruction)
{
    // LDH [0xFF00 + C], A
    uint16_t addr = 0xFF00 | regs.C;
    writeMem8(addr, regs.A);
    // no flags affected
    return false;
}

static bool ldh_a_mc(const Instruction instruction)
{
    // LDH A, [0xFF00 + C]
    uint16_t addr = 0xFF00 | regs.C;
    regs.A = readMem8(addr);
    // no flags affected
    return false;
}

static bool ldh_ma8_a(const Instruction instruction)
{
    // LDH [0xFF00 + imm8], A
    uint16_t addr = 0xFF00 | readImm8();
    writeMem8(addr, regs.A);
    // no flags affected
    return false;
}

static bool ldh_a_ma8(const Instruction instruction)
{
    // LDH A, [0xFF00 + imm8]
    uint16_t addr = 0xFF00 | readImm8();
    regs.A = readMem8(addr);
    // no flags affected
    return false;
}

static bool ld_ma16_a(const Instruction instruction)
{
    // LDH [imm16], A
    uint16_t addr = readImm16();
    writeMem8(addr, regs.A);
    // no flags affected
    return false;
}

static bool ld_a_ma16(const Instruction instruction)
{
    // LDH A, [imm16]
    uint16_t addr = readImm16();
    regs.A = readMem8(addr);
    // no flags affected
    return false;
}

static bool add_sp_e8(const Instruction instruction)
{
    // ADD SP, imm8
    int8_t val8 = (int8_t)readImm8();
    uint16_t result16 = val8 + regs.SP;
    uint16_t result8 = (val8 & 0xFF) + (regs.SP & 0x00FF);
    uint8_t halfResult = (val8 & 0x0F) + (regs.SP & 0x000F);
    cpuCycles(2);  // 16 bit addition through ALU takes extra cycle, and getting result back to SP is another
    regs.SP = result16;
    regs.flags.zero = 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = (halfResult & 0x10) >> 4;
    regs.flags.carry = (result8 & 0x0100) >> 8;
    return false;
}

static bool ld_hl_spe8(const Instruction instruction)
{
    // LD HL, SP+imm8
    int8_t val8 = (int8_t)readImm8();
    uint16_t result16 = val8 + regs.SP;
    uint16_t result8 = (val8 & 0xFF) + (regs.SP & 0x00FF);
    uint8_t halfResult = (val8 & 0x0F) + (regs.SP & 0x000F);
    cpuCycle();  // 16 bit add through ALU takes extra cycle
    regs.HL = result16;
    regs.flags.zero = 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = (halfResult & 0x10) >> 4;
    regs.flags.carry = (result8 & 0x0100) >> 8;
    return false;
}

static bool ld_sp_hl(const Instruction instruction)
{
    // LD SP, HL
    regs.SP = regs.HL;
    cpuCycle(); // Takes an extra cycle to move from SP
    // no flags affected
    return false;
}


static bool di(const Instruction instruction)
{
    // DI
    interruptsEnabled = false;
    interruptsPendingEnable = false;
    // no flags affected
    return false;
}

static bool ei(const Instruction instruction)
{
    // EI
    interruptsPendingEnable = true;
    // no flags affected
    return false;
}

static bool sla_r8(const Instruction instruction)
{
    // SLA r8
    int8_t val8 = (int8_t)getReg8(instruction.r8_op);
    int8_t result8 = (val8 << 1);
    setReg8(instruction.r8_op, (uint8_t)result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = ((val8 & 0x80) >> 7);
    return false;
}

static bool sra_r8(const Instruction instruction)
{
    // SRA r8
    int8_t val8 = (int8_t)getReg8(instruction.r8_op);
    int8_t result8 = (val8 >> 1);
    setReg8(instruction.r8_op, (uint8_t)result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x01);
    return false;
}

static bool srl_r8(const Instruction instruction)
{
    // SRL r8
    uint8_t val8 = getReg8(instruction.r8_op);
    uint8_t result8 = (val8 >> 1);
    setReg8(instruction.r8_op, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x01);
    return false;
}

static bool swap_r8(const Instruction instruction)
{
    // SWAP r8
    uint8_t val8 = getReg8(instruction.r8_op);
    uint8_t result8 = ((val8 & 0x0f) << 4) | ((val8 & 0xf0) >> 4);
    setReg8(instruction.r8_op, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = 0;
    return false;
}

static bool bit_bit_r8(const Instruction instruction)
{
    regs.flags.zero = (0 == (getReg8(instruction.r8_op) & (0x01<<instruction.bit))) ? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 1;
    // carry flag not affected
    return false;
}

static bool res_bit_r8(const Instruction instruction)
{
    setReg8(instruction.r8_op, (getReg8(instruction.r8_op) & ~(0x01<<instruction.bit)));
    // no flags affected
    return false;
}

static bool set_bit_r8(const Instruction instruction)
{
    setReg8(instruction.r8_op, (getReg8(instruction.r8_op) | (0x01<<instruction.bit)));
    // no flags affected
    return false;
}


typedef bool (doOp)(const Instruction instruction);

static bool prefix(const Instruction instruction)
{
    static doOp * const prefixBlock0Decode[8] = {
        rlc_r8,         rrc_r8,         rl_r8,          rr_r8,      sla_r8,         sra_r8,     swap_r8,    srl_r8
    };

    const Instruction pfx_inst = {.val = readMem8(regs.PC++)};
    switch( pfx_inst.block ) {
        case 0:
            return prefixBlock0Decode[pfx_inst.op](pfx_inst);
        case 1:
            return bit_bit_r8(pfx_inst);
        case 2:
            return res_bit_r8(pfx_inst);
        case 3:
            return set_bit_r8(pfx_inst);
    }
    return false;  // impossible, but silence warning
}

bool executeInstruction(const uint16_t breakpoint)
{
    Instruction instruction = nextInstruction;

    static doOp * const block0decode[64] = {
        nop,            ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   rlc_a,
        ld_ma16_sp,     add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   rrc_a,

        stop,           ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   rl_a,
        jr_e8,          add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   rr_a,

        jr_cc_e8,       ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   daa,
        jr_cc_e8,       add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   cpl,

        jr_cc_e8,       ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   scf,
        jr_cc_e8,       add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   ccf,
    };

    static doOp * const block3decode[64] = {
        ret_cc,         pop_r16,        jp_cc_i16,      jp_i16,     call_cc_i16,    push_r16,   alu_i8,     rst,
        ret_cc,         ret,            jp_cc_i16,      prefix,     call_cc_i16,    call_i16,   alu_i8,     rst,

        ret_cc,         pop_r16,        jp_cc_i16,      invalid,    call_cc_i16,    push_r16,   alu_i8,     rst,
        ret_cc,         reti,           jp_cc_i16,      invalid,    call_cc_i16,    invalid,    alu_i8,     rst,

        ldh_ma8_a,      pop_r16,        ldh_mc_a,       invalid,    invalid,        push_r16,   alu_i8,     rst,
        add_sp_e8,      jp_hl,          ld_ma16_a,      invalid,    invalid,        invalid,    alu_i8,     rst,

        ldh_a_ma8,      pop_r16,        ldh_a_mc,       di,         invalid,        push_r16,   alu_i8,     rst,
        ld_hl_spe8,     ld_sp_hl,       ld_a_ma16,      ei,         invalid,        invalid,    alu_i8,     rst,
    };

    if( cpuHalted ) {
        cpuCycle();
        return false;
    }

    // Test and handle any pending interrupts!
    if( interruptsEnabled ) {
        // is anything pending?
        InterruptRegister pending;
        pending.flags = (ieReg.flags & ifReg.flags);
        if(0 != pending.flags) {
            cpuCycle(); // 1 cycle to decrement PC, 1 cycle to pre-decrement SP in push below
            push16(regs.PC-1);  // the SP pre-decrement cycle, then 2 cycles to write out PC
            // recalculate what is pending in case anything of higher priorty fired during the last few cycles
            pending.flags = (ieReg.flags & ifReg.flags);
            // handle in priority order
            if( pending.vblank ) {
                regs.PC = 0x40;
                ifReg.vblank = 0;
            } else if( pending.stat ) {
                regs.PC = 0x48;
                ifReg.stat = 0;
            } else if( pending.timer ) {
                regs.PC = 0x50;
                ifReg.timer = 0;
            } else if( pending.serial ) {
                regs.PC = 0x58;
                ifReg.serial = 0;
            } else if( pending.joypad ) {
                regs.PC = 0x60;
                ifReg.joypad = 0;
            }
            instruction.val = readMem8(regs.PC++);

            interruptsEnabled = false;
            interruptsPendingEnable = false;
        }
    } else {
        // latch any pending re-enable
        interruptsEnabled = interruptsPendingEnable;
    }

    // keep a record of the executed instructions for the disassembly view
    instructionHistory[historyHead].addr = regs.PC-1;
    instructionHistory[historyHead].code[0] = getMem8(regs.PC-1);
    instructionHistory[historyHead].code[1] = getMem8(regs.PC);
    instructionHistory[historyHead].code[2] = getMem8(regs.PC+1);
    historyHead = (historyHead+1) & 0x7;

    bool hung;
    switch( instruction.block ) {
        case INST_BLOCK0:
            hung = block0decode[instruction.decode](instruction);
            break;
        case INST_BLOCK1:
            // Mostly register to register loads
            if(INST_HALT == instruction.val) {
                hung = halt(instruction);
            } else {
                hung = ld_r8_r8(instruction);
            }
            break;
        case INST_BLOCK2:
            // all register based ALU operations
            hung = alu_r8(instruction);
            break;
        case INST_BLOCK3:
            hung = block3decode[instruction.decode](instruction);
            break;
    }

    if( NULL != doctorLogFile ) {
        extern bool bootRomActive;
        if(!bootRomActive) {
            // A:00 F:11 B:22 C:33 D:44 E:55 H:66 L:77 SP:8888 PC:9999 PCMEM:AA,BB,CC,DD
            fprintf(doctorLogFile,
                "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n",
                regs.A, regs.F, regs.B, regs.C, regs.D, regs.E, regs.H, regs.L, regs.SP, regs.PC,
                getMem8(regs.PC), getMem8(regs.PC+1), getMem8(regs.PC+2), getMem8(regs.PC+3)
            );
        }
    }

    nextInstruction.val = readMem8(regs.PC++);

    return (breakpoint == (regs.PC-1)) || hung;
}

// verification test the mooneye test suite uses to indicate a test has passed
bool mooneyeSuccess(void)
{
    // Fibonacci sequence in regs BCDEHL
    return (3 == regs.B) && (5 == regs.C) && (8 == regs.D) && (13 == regs.E) && (21 == regs.H) && (34 == regs.L);
}

#include "gb.h"
#include "gui.h"

static struct __attribute__((packed)) {
    union {
        uint16_t AF;
        struct {
            union {
                uint8_t F;
                struct {
                    uint8_t zero:1;
                    uint8_t sub:1;
                    uint8_t halfCarry:1;
                    uint8_t carry:1;
                    uint8_t reserved:4;
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

static uint8_t nextInstruction = 0; // start with NOP at boot

bool breakpoint(void)
{
    return (0x0022 == regs.PC);
}

void resetCpu(void)
{
    memset(&regs, 0, sizeof(regs));
    nextInstruction = getMem8(regs.PC++);
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

extern RomImage bootrom; // TODO get rid of this

void guiDrawCpuState(void)
{
    // Top left corner of the memory viewer
    Vector2 viewAnchor1 = { 20, 100 };

    Vector2 regAnchor1 = { viewAnchor1.x, viewAnchor1.y };
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
    uint16_t flags = regs.flags.zero << 12 | regs.flags.sub << 8 | regs.flags.halfCarry << 4 | regs.flags.carry << 12;
    guiDrawCpuReg16(regAnchor1, flags, "Z N H C");
    regAnchor1.y += FONTSIZE*2;

    char buffer[1024];
    char instBuffer[32];
    char *buff, *instBuff;
    int offset = regs.PC-1;
    int jumpDest, bytesPerInst, lines=0;

    buff = buffer;
    do {
        buff = buff + sprintf(buff, "%04X | ", offset);
        instBuff = instBuffer;
        bytesPerInst = disassembleInstruction(&bootrom, offset, &instBuff, &jumpDest);
        for(int index = 0; index < 3; index++) {
            if (index < bytesPerInst) {
                buff = buff + sprintf(buff, "%02X ", bootrom.contents[offset+index]);
            } else {
                buff = buff + sprintf(buff,"   ");
            }
        }
        buff = buff + sprintf(buff, "|  %s\n", instBuffer);
        offset += bytesPerInst;
    } while( ++lines < 4 );
    DrawTextEx(firaFont, buffer, regAnchor1, FONTSIZE, 0, BLACK);

    /*
    DrawTextEx(firaFont, TextFormat("%04X |", regs.PC-1),regAnchor1, FONTSIZE, 0, BLACK);
    regAnchor1.x += FONTWIDTH*7;
    int bytesPerInst = disassembleInstruction(&bootrom, regs.PC-1, buffer, &jumpDest);
    for(int index = 0; index < 3; index++) {
        if (index < bytesPerInst) {
            DrawTextEx(firaFont, TextFormat("%02X", bootrom.contents[regs.PC-1+index]), regAnchor1, FONTSIZE, 0, BLACK);
        }
        regAnchor1.x += FONTWIDTH*3;
    }
    DrawTextEx(firaFont, "|", regAnchor1, FONTSIZE, 0, BLACK);
    regAnchor1.x += FONTWIDTH*2;
    DrawTextEx(firaFont, buffer,  regAnchor1, FONTSIZE, 0, BLACK);
    */
}


bool checkCond(uint8_t cc)
{
    switch(cc) {
        case 0: // NZ
            return (regs.flags.zero == 0);
        case 1: // Z
            return (regs.flags.zero == 1);
        case 2: // NC
            return (regs.flags.carry == 0);
        case 3: // C
            return (regs.flags.carry == 1);
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

uint8_t getReg8(uint8_t r8)
{
    if( r8 == R8_HL ) {
        return readMem8(regs.HL);
    } else {
        return *r8_regs[r8];
    }
}

void setReg8(uint8_t r8, uint8_t val8)
{
    if( r8 == R8_HL ) {
        writeMem8(regs.HL, val8);
    } else {
        *r8_regs[r8] = val8;
    }
}

uint16_t getReg16(uint8_t r16)
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
    }
}

void setReg16(uint8_t r16, uint16_t val16)
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

uint8_t readMem8R16(uint8_t r16)
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
    }
}

void writeMem8R16(uint8_t r16, uint16_t val8)
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

uint16_t __inline pop16(void)
{
    uint16_t val16 = readMem8(regs.SP++);
    val16 |= readMem8(regs.SP++) << 8;
    return val16;
}

void __inline push16(uint16_t val16)
{
    cpuCycles(1); // pre-decrement takes a cycle
    writeMem8(--regs.SP, MSB(val16));
    writeMem8(--regs.SP, LSB(val16));
}


void nop(uint8_t instruction)
{
    // Nothing to see here
    // no flags affected
}

void ld_ma16_sp(uint8_t instruction)
{
    // LD [imm16], SP
    uint16_t addr = readMem16(regs.PC);
    regs.PC += 2;
    writeMem16(addr, regs.SP);
    // no flags affected
}

void stop(uint8_t instruction)
{
    // TODO
}

void jr_e8(uint8_t instruction)
{
    // JR imm8
    int8_t offset = (int8_t)readMem8(regs.PC++);
    cpuCycles(1);
    regs.PC = offset + regs.PC;
    // no flags affected
}

void jr_cc_e8(uint8_t instruction)
{
    // JR cc, imm8
    int8_t offset = (int8_t)readMem8(regs.PC++);
    if( checkCond(INST_COND_EXTRACT(instruction)) ) {
        cpuCycles(1);
        regs.PC = offset + regs.PC;
    }
    // no flags affected
}

void ld_r16_i16(uint8_t instruction)
{
    // LD r16, imm16
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    setReg16(r16, readMem16(regs.PC));
    regs.PC += 2;
    // no flags affected
}

void add_hl_r16(uint8_t instruction)
{
    // ADD HL, r16
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    uint32_t val16 = getReg16(r16);
    uint32_t result = val16 + regs.HL;
    uint32_t halfResult = (val16 & 0x0FFF) + (regs.HL & 0x0FFF);
    // Takes an extra cycle since this is executed as 2 separate 8 bit adds
    cpuCycles(1);
    // zero flag not affected
    regs.flags.sub = 0;
    regs.flags.halfCarry = (0x00001000&halfResult)>>12;
    regs.flags.carry = (0x00010000&result)>>16;
}

void ld_mr16_a(uint8_t instruction)
{
    // LD [r16mem], A
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    writeMem8R16(r16, regs.A);
    // no flags affected
}

void ld_a_mr16(uint8_t instruction)
{
    // LD A, [r16mem]
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    regs.A = readMem8R16(r16);
    // no flags affected
}

void inc_r16(uint8_t instruction)
{
    // INC r16
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    setReg16(r16, getReg16(r16)+1);
    // Takes an extra cycle because the IDU is used for the increment/decrement
    cpuCycles(1);
    // no flags affected

}
void dec_r16(uint8_t instruction)
{
    // DEC
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    setReg16(r16, getReg16(r16)-1);
    // Takes an extra cycle because the IDU is used for the increment/decrement
    cpuCycles(1);
    // no flags affected

}

void inc_r8(uint8_t instruction)
{
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    uint8_t v8 = getReg8(r8_a);
    uint8_t result= v8+1;
    uint8_t halfResult = (v8&0xF)+1;
    setReg8(r8_a, result);
    regs.flags.zero = (0==result)?1:0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = (0x0010&halfResult)>>4;
    // carry not affected
}

void dec_r8(uint8_t instruction)
{
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    uint8_t v8 = getReg8(r8_a);
    uint8_t result= v8-1;
    uint8_t halfResult = (v8&0xF)-1;
    setReg8(r8_a, result);
    regs.flags.zero = (0==result)?1:0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = (0x0010&halfResult)>>4;
    // carry not affected
}

void ld_r8_i8(uint8_t instruction)
{
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    setReg8(r8_a,readMem8(regs.PC++));
    // no flags affected
}

void rlc_r8(uint8_t instruction)
{
    // RLC r8
    const uint8_t r8 = INST_R8_B_EXTRACT(instruction);
    uint8_t val8 = getReg8(r8);
    uint8_t result8 = (val8 << 1) | ((val8 & 0x80) >> 7);
    setReg8(r8, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = ((val8 & 0x80) >> 7);
}

void rlc_a(uint8_t instruction)
{
    // RLCA
    // this version of always operates on reg A and leaves zero flag clear
    rlc_r8(instruction);
    regs.flags.zero = 0;
    // other flags adjusted above
}

void rl_r8(uint8_t instruction)
{
    // RL r8
    const uint8_t r8 = INST_R8_B_EXTRACT(instruction);
    uint8_t val8 = getReg8(r8);
    uint8_t result8 = (val8 << 1) | regs.flags.carry;
    setReg8(r8, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x80) >> 7;
}

void rl_a(uint8_t instruction)
{
    // RLA
    // this version of always operates on reg A and leaves zero flag clear
    rl_r8(instruction);
    regs.flags.zero = 0;
    // other flags adjusted above
}

void rrc_r8(uint8_t instruction)
{
    // RRC r8
    const uint8_t r8    = INST_R8_B_EXTRACT(instruction);
    uint8_t val8 = getReg8(r8);
    uint8_t result8 = ((val8 & 0x01) << 7) | (val8 >> 1);
    setReg8(r8, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x01);
}

void rrc_a(uint8_t instruction)
{
    // RRCA
    // this version of always operates on reg A and leaves zero flag clear
    rrc_r8(instruction);
    regs.flags.zero = 0;
    // other flags adjusted above
}

void rr_r8(uint8_t instruction)
{
    // RR r8
    const uint8_t r8    = INST_R8_B_EXTRACT(instruction);
    uint8_t val8 = getReg8(r8);
    uint8_t result8 = (regs.flags.carry << 7) | (val8 >> 1);
    setReg8(r8, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x01);
}

void rr_a(uint8_t instruction)
{
    // RRA
    // this version of always operates on reg A and leaves zero flag clear
    rr_r8(instruction);
    regs.flags.zero = 0;
    // other flags adjusted above
}

void daa(uint8_t instruction)
{
    // DAA
    uint8_t adj;
    int16_t result16 = regs.A;
    if(regs.flags.sub) {
        adj = (regs.flags.halfCarry)? 0x06 : 0x00;
        adj += (regs.flags.carry)? 0x60 : 0x00;
        result16 -= adj;

    } else {
        adj = ((regs.flags.halfCarry) || ((regs.A&0x0F) > 0x09))? 0x06 : 0x00;
        adj += ((regs.flags.carry) || (regs.A>0x99)) ? 0x60 : 0x00;
        result16 += adj;
    }
    regs.flags.zero = (0 == regs.A)?1:0;
    // sub flag not affected
    regs.flags.halfCarry = 0;
    regs.flags.carry = (0x0100&result16)>>8;
}

void cpl(uint8_t instruction)
{
    // CPL
    // compliment A
    regs.A = ~regs.A;
    // zero flag not affected
    regs.flags.sub = 1;
    regs.flags.halfCarry = 1;
    // carry flag not affected
}

void scf(uint8_t instruction)
{
    // SCF
    // set carry flag
    // zero flag not affected
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = 1;
}

void ccf(uint8_t instruction)
{
    // CCF
    // compliment carry flag
    // zero flag not affected
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = ~regs.flags.carry;
}

static __inline void alu(uint8_t op, uint8_t val8)
{
    int16_t A = regs.A;
    int16_t result;
    int16_t halfResult = 0;
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
            break;
        case 5: // XOR
            result= A^val8;
            break;
        case 6: // OR
            result= A|val8;
            break;
        case 7: // CP
            result= A-val8;
            break;
    }
    if( 7 != op ) {
        regs.A = result;
    }
    regs.flags.zero = (0==result)?1:0;
    // sub flag adjusted above
    regs.flags.carry = (0x0100&result)>>8;
    regs.flags.halfCarry = (0x0010&halfResult)>>4;
}

void alu_r8(uint8_t instruction)
{
    // ALU A, r8
    const uint8_t op  = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8_b  = INST_R8_B_EXTRACT(instruction);

    uint8_t val8 = getReg8(r8_b);
    alu(op, val8);
    // flags adjusted in alu helper
}

void halt(uint8_t instruction)
{
    // TODO
}

void ld_r8_r8(uint8_t instruction)
{
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8_b  = INST_R8_B_EXTRACT(instruction);
    setReg8(r8_a,getReg8(r8_b));
    // no flags affected
}

void pop_r16(uint8_t instruction)
{
    // POP r16
    const uint8_t r16 = INST_R16_EXTRACT(instruction);
    uint16_t val16 = pop16();
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
            regs.AF = val16;
            break;
    }
    // AF affects flags, rest do not
}

void push_r16(uint8_t instruction)
{
    // PUSH r16
    const uint8_t r16 = INST_R16_EXTRACT(instruction);
    switch(r16) {
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
}

void jp_i16(uint8_t instruction)
{
    // JP imm16
    uint16_t pc = (readMem16(regs.PC));
    regs.PC += 2;
    cpuCycles(1);   // extra cycle to transfer to PC
    regs.PC = pc;
    // no flags affected
}

void jp_cc_i16(uint8_t instruction)
{
    // JP cond, imm16
    uint16_t pc = (readMem16(regs.PC));
    regs.PC += 2;
    if( checkCond(INST_COND_EXTRACT(instruction)) ) {
        cpuCycles(1);   // extra cycle to transfer to PC
        regs.PC = pc;
    }
    // no flags affected
}

void jp_hl(uint8_t instruction)
{
    // JP HL
    regs.PC = regs.HL;
    // no flags affected
}

void call_i16(uint8_t instruction)
{
    // CALL imm16
    uint16_t pc = (readMem16(regs.PC));
    regs.PC += 2;
    push16(regs.PC);
    regs.PC = pc;
    // no flags affected
}

void call_cc_i16(uint8_t instruction)
{
    // CALL cond, imm16
    uint16_t pc = (readMem16(regs.PC));
    regs.PC += 2;
    if( checkCond(INST_COND_EXTRACT(instruction)) ) {
        push16(regs.PC);
        regs.PC = pc;
    }
    // no flags affected
}

void rst(uint8_t instruction)
{
    // RST vec
    push16(regs.SP);
    regs.PC = INST_RST_TGT3_EXTRACT(instruction) * 8;
    // no flags affected
}

void ret(uint8_t instruction)
{
    // RET
    uint16_t pc = pop16();
    cpuCycles(1);  // extra cycle to move to PC
    regs.PC = pc;
    // no flags affected
}

void ret_cc(uint8_t instruction)
{
    // RET cond
    cpuCycles(1); // takes a cycle to test conditions
    if( checkCond(INST_COND_EXTRACT(instruction)) ) {
        ret(instruction);
    }
    // no flags affected
}

void reti(uint8_t instruction)
{
    // RETI
    ret(instruction);
    // TODO interrupt enable
    // no flags affected
}

void alu_i8(uint8_t instruction)
{
    // ALU A, imm8
    const uint8_t op  = INST_R8_A_EXTRACT(instruction);
    uint8_t val8 = readMem8(regs.PC++);
    alu(op, val8);
    // flags adjusted in ALU helper
}

void invalid(uint8_t instruction)
{
    // INVALID INSTRUCTION
    // TODO
    // no flags affected
}

void ldh_mc_a(uint8_t instruction)
{
    // LDH [0xFF00 + C], A
    uint16_t addr = 0xFF00 | regs.C;
    writeMem8(addr, regs.A);
    // no flags affected
}

void ldh_a_mc(uint8_t instruction)
{
    // LDH A, [0xFF00 + C]
    uint16_t addr = 0xFF00 | regs.C;
    regs.A = readMem8(addr);
    // no flags affected
}

void ldh_ma8_a(uint8_t instruction)
{
    // LDH [0xFF00 + imm8], A
    uint16_t addr = 0xFF00 | readMem8(regs.PC++);
    writeMem8(addr, regs.A);
    // no flags affected
}

void ldh_a_ma8(uint8_t instruction)
{
    // LDH A, [0xFF00 + imm8]
    uint16_t addr = 0xFF00 | readMem8(regs.PC++);
    regs.A = readMem8(addr);
    // no flags affected
}

void ld_ma16_a(uint8_t instruction)
{
    // LDH [imm16], A
    uint16_t addr = readMem16(regs.PC);
    regs.PC += 2;
    writeMem8(addr, regs.A);
    // no flags affected
}

void ld_a_ma16(uint8_t instruction)
{
    // LDH A, [imm16]
    uint16_t addr = readMem16(regs.PC);
    regs.PC += 2;
    regs.A = readMem8(addr);
    // no flags affected
}

void add_sp_e8(uint8_t instruction)
{
    // ADD SP, imm8
    int8_t val8 = (int8_t)readMem8(regs.PC++);
    uint16_t result = val8 + regs.SP;
    int8_t halfResult = (val8 & 0x0F) + (regs.SP & 0x000F);
    cpuCycles(2);  // 16 bit addition through ALU takes extra cycle, and getting result back to SP is another
    regs.SP = result;
    regs.flags.zero = 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = (halfResult & 0x0010) >> 4;
    regs.flags.carry = (result & 0x0100) >> 8;
}

void ld_sp_hl(uint8_t instruction)
{
    // LD SP, HL
    regs.SP = regs.HL;
    cpuCycles(1); // Takes an extra cycle to move from SP
    // no flags affected
}

void ld_hl_spe8(uint8_t instruction)
{
    // LD HL, SP+imm8
    int8_t val8 = (int8_t)readMem8(regs.PC++);
    uint16_t result = val8 + regs.SP;
    int8_t halfResult = (val8 & 0x0F) + (regs.SP & 0x000F);
    cpuCycles(1);  // 16 bit add through ALU takes extra cycle
    regs.HL = result;
    regs.flags.zero = 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = (halfResult & 0x0010) >> 4;
    regs.flags.carry = (result & 0x0100) >> 8;

}

void di(uint8_t instruction)
{
    // DI
    // TODO
    // no flags affected
}

void ei(uint8_t instruction)
{
    // EI
    // TODO
    // no flags affected
}

void sla_r8(uint8_t instruction)
{
    // SLA r8
    const uint8_t r8 = INST_R8_B_EXTRACT(instruction);
    int8_t val8 = (int8_t)getReg8(r8);
    int8_t result8 = (val8 << 1);
    setReg8(r8, (uint8_t)result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = ((val8 & 0x80) >> 7);
}

void sra_r8(uint8_t instruction)
{
    // SRA r8
    const uint8_t r8 = INST_R8_B_EXTRACT(instruction);
    int8_t val8 = (int8_t)getReg8(r8);
    int8_t result8 = (val8 >> 1);
    setReg8(r8, (uint8_t)result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x01);
}

void swap_r8(uint8_t instruction)
{
    // SWAP r8
    const uint8_t r8 = INST_R8_B_EXTRACT(instruction);
    uint8_t val8 = getReg8(r8);
    uint8_t result8 = ((val8 & 0x0f) << 4) | ((val8 & 0xf0) >> 4);
    setReg8(r8, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = 0;
}

void srl_r8(uint8_t instruction)
{
    // SRA r8
    const uint8_t r8 = INST_R8_B_EXTRACT(instruction);
    uint8_t val8 = getReg8(r8);
    uint8_t result8 = (val8 >> 1);
    setReg8(r8, result8);
    regs.flags.zero = (result8 == 0)? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 0;
    regs.flags.carry = (val8 & 0x01);
}

void bit_bit_r8(uint8_t instruction)
{
    const uint8_t bit   = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8    = INST_R8_B_EXTRACT(instruction);
    regs.flags.zero = (0 == (getReg8(r8) & (0x01<<bit))) ? 1 : 0;
    regs.flags.sub = 0;
    regs.flags.halfCarry = 1;
    // carry flag not affected
}

void res_bit_r8(uint8_t instruction)
{
    const uint8_t bit   = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8    = INST_R8_B_EXTRACT(instruction);
    setReg8(r8, (getReg8(r8) & ~(0x01<<bit)));
    // no flags affected
}

void set_bit_r8(uint8_t instruction)
{
    const uint8_t bit   = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8    = INST_R8_B_EXTRACT(instruction);
    setReg8(r8, (getReg8(r8) | (0x01<<bit)));
    // no flags affected
}


typedef void (doOp)(uint8_t instruction);

void prefix(uint8_t instruction)
{
    static doOp * const prefixBlock0Decode[8] = {
        rlc_r8,         rrc_r8,         rl_r8,          rr_r8,      sla_r8,         sra_r8,     swap_r8,    srl_r8
    };

    const uint8_t pfx_inst = readMem8(regs.PC++);
    const uint8_t pfx_block = INST_BLOCK_EXTRACT(pfx_inst);
    switch( pfx_block ) {
        case 0:
            prefixBlock0Decode[INST_R8_A_EXTRACT(pfx_inst)](pfx_inst);
            break;
        case 1:
            bit_bit_r8(pfx_inst);
            break;
        case 2:
            res_bit_r8(pfx_inst);
            break;
        case 3:
            set_bit_r8(pfx_inst);
            break;
    }
}

void executeInstruction(void)
{
    const uint8_t instruction = nextInstruction;

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

    const uint8_t block = INST_BLOCK_EXTRACT(instruction);

    switch( block ) {
        case INST_BLOCK0:
            block0decode[(instruction & ~INST_BLOCK_MASK)](instruction);
            break;
        case INST_BLOCK1:
            // Mostly register to register loads
            if(INST_HALT == instruction) {
                halt(instruction);
            } else {
                ld_r8_r8(instruction);
            }
            break;
        case INST_BLOCK2:
            // all register based ALU operations
            alu_r8(instruction);
            break;
        case INST_BLOCK3:
            block3decode[(instruction & ~INST_BLOCK_MASK)](instruction);
            break;
    }

    nextInstruction = readMem8(regs.PC++);
}

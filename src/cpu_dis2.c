#include "gb.h"

static const char * const r8Decode[] = {
    "B",
    "C",
    "D",
    "E",
    "H",
    "L",
    "[HL]",
    "A",
};

static const char * const r16Decode[] = {
    "BC",
    "DE",
    "HL",
    "SP",
};

static const char * const r16StkDecode[] = {
    "BC",
    "DE",
    "HL",
    "AF",
};

static const char * const r16MemDecode[] = {
    "BC",
    "DE",
    "HL+",
    "HL-",
};

static const char * const condDecode[] = {
    "NZ",
    "Z",
    "NC",
    "C",
};

static const char * const aluDecode[8] = {
    "ADD ",
    "ADC ",
    "SUB ",
    "SBC ",
    "AND ",
    "XOR ",
    "OR  ",
    "CP  ",
};

static const char * const aluFlagDecode[8] = {
    "z0ch",
    "z0ch",
    "z1ch",
    "z1ch",
    "z0c1",
    "z0c0",
    "z0c0",
    "z1ch",
};


static int nop(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // NOP
    // flags ----
    return sprintf(buff, "NOP");
}

static int ld_ma16_sp(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD [imm16], SP
    // flags ----
    return sprintf(buff, "LD   [0x%04X], SP", MEM_IMM16_EXTRACT(code, 0));
}

static int stop(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // STOP
    // flags ----
    return sprintf(buff, "STOP");
}

static int jr_e8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // JR imm8
    // flags ----
    uint16_t dest = addr+2 + ((int8_t)code[1]);
    return sprintf(buff, "JR   LABEL_%04X", dest);
}

static int jr_cc_e8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // JR cc, imm8
    // flags ----
    const uint8_t cond  = INST_COND_EXTRACT(instruction);
    uint16_t dest = addr+2 + ((int8_t)code[1]);
    return sprintf(buff, "JR   %s, LABEL_%04X", condDecode[cond], dest);
}

static int ld_r16_i16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD r16, imm16
    // flags ----
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    return sprintf(buff, "LD   %s, 0x%04X", r16Decode[r16], MEM_IMM16_EXTRACT(code, 0));
}

static int add_hl_r16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // ADD HL, r16
    // flags -0hc
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    return sprintf(buff, "ADD  HL, %s", r16Decode[r16]);
}

static int ld_mr16_a(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD [r16mem], A
    // flags ----
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    return sprintf(buff, "LD   [%s], A", r16MemDecode[r16]);
}

static int ld_a_mr16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD A, [r16mem]
    // flags ----
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    return sprintf(buff, "LD   A, [%s]", r16MemDecode[r16]);
}

static int inc_r16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // INC r16
    // flags ----
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    return sprintf(buff, "INC  %s", r16Decode[r16]);
}
static int dec_r16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // DEC r16
    // flags ----
    const uint8_t r16   = INST_R16_EXTRACT(instruction);
    return sprintf(buff, "DEC  %s", r16Decode[r16]);
}

static int inc_r8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // INC r8
    // flags z0h-
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    return sprintf(buff, "INC  %s", r8Decode[r8_a]);
}

static int dec_r8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // DEC r8
    // flags z0h-
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    return sprintf(buff, "DEC  %s", r8Decode[r8_a]);
}

static int ld_r8_i8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD r8, imm8
    // flags ----
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    return sprintf(buff, "LD   %s, 0x%02X", r8Decode[r8_a], code[1]);
}

static int bitop_a(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    static const char * const bitopDecode[8] = {
        "RLCA",
        "RRCA",
        "RLA ",
        "RRA ",
        "DAA ",
        "CPL ",
        "SCF ",
        "CCF ",
    };
    static const char * const bitopFlagDecode[8] = {
        "000c",
        "000c",
        "000c",
        "000c",
        "z-0c",
        "-11-",
        "-001",
        "-00c",
    };
    const uint8_t op_a  = INST_R8_A_EXTRACT(instruction);
    return sprintf(buff, "%s", bitopDecode[op_a]);
}

static int alu_r8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // ALU A, r8
    // flag decode provided above
    const uint8_t op_a  = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8_b  = INST_R8_B_EXTRACT(instruction);
    return sprintf(buff, "%s A, %s", aluDecode[op_a], r8Decode[r8_b]);
}

static int halt(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // HALT
    // flags ----
    return sprintf(buff, "HALT");
}

static int ld_r8_r8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD r8, r8
    // flags ----
    const uint8_t r8_a  = INST_R8_A_EXTRACT(instruction);
    const uint8_t r8_b  = INST_R8_B_EXTRACT(instruction);
    return sprintf(buff, "LD   %s, %s", r8Decode[r8_a], r8Decode[r8_b]);
}

static int pop_r16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // POP r16
    const char * const popFlagDecode[4] = {
        "znhc",
        "----",
        "----",
        "----",
    };
    const uint8_t r16 = INST_R16_EXTRACT(instruction);
    return sprintf(buff, "POP  %s", r16StkDecode[r16]);
}

static int push_r16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // PUSH r16
    // flags ----
    const uint8_t r16 = INST_R16_EXTRACT(instruction);
    return sprintf(buff, "PUSH %s", r16StkDecode[r16]);
}

static int jp_i16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // JP imm16
    // flags ----
    const uint16_t dest = MEM_IMM16_EXTRACT(code, 0);
    return sprintf(buff, "JP   LABEL_%04X", dest);
}

static int jp_cc_i16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // JP cond, imm16
    // flags ----
    const uint8_t cond  = INST_COND_EXTRACT(instruction);
    const uint16_t dest = MEM_IMM16_EXTRACT(code, 0);
    return sprintf(buff, "JP   %s, LABEL_%04X", condDecode[cond], dest);
}

static int jp_hl(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // JP HL
    // flags ----
    return sprintf(buff, "JP   HL");
    // can't determine jump dest address unless running code
}

static int call_i16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // CALL imm16
    // flags ----
    const uint16_t dest = MEM_IMM16_EXTRACT(code, 0);
    return sprintf(buff, "CALL LABEL_%04X", dest);
}

static int call_cc_i16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // CALL cond, imm16
    // flags ----
    const uint8_t cond  = INST_COND_EXTRACT(instruction);
    const uint16_t dest = MEM_IMM16_EXTRACT(code, 0);
    return sprintf(buff, "CALL %s, LABEL_%04X", condDecode[cond], dest);
}

static int rst(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // RST vec
    // flags ----
    const uint16_t dest = INST_RST_TGT3_EXTRACT(instruction) * 8;
    return sprintf(buff, "RST 0x%02X (LABEL_%04X)", dest, dest);
}

static int ret(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // RET
    // flags ----
    return sprintf(buff, "RET");
}

static int ret_cc(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // RET cond
    // flags ----
    const uint8_t cond  = INST_COND_EXTRACT(instruction);
    return sprintf(buff, "RET  %s", condDecode[cond]);
}

static int reti(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // RETI
    // flags ----
    return sprintf(buff, "RETI");
}

static int alu_i8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // ALU A, imm8
    // flag decode provided above
    const uint8_t op_a  = INST_R8_A_EXTRACT(instruction);
    return sprintf(buff, "%s A, 0x%02X", aluDecode[op_a], code[1]);
}

static int invalid(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // INVALID INSTRUCTION
    // flags ----
    return sprintf(buff, "INVALID");
}

static int ldh_mc_a(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LDH [0xFF00 + C], A
    // flags ----
    return sprintf(buff, "LDH  [0xFF00+C], A");
}

static int ldh_a_mc(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LDH A, [0xFF00 + C]
    // flags ----
    return sprintf(buff, "LDH  A, [0xFF00+C]");
}

static int ldh_ma8_a(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LDH [0xFF00 + imm8], A
    // flags ----
    return sprintf(buff, "LDH  [0x%04X], A", MEM_IMM8_H_TO_ADDR(code, 0));
}

static int ldh_a_ma8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LDH A, [0xFF00 + imm8]
    // flags ----
    return sprintf(buff, "LDH  A, [0x%04X]", MEM_IMM8_H_TO_ADDR(code, 0));
}

static int ld_ma16_a(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD [imm16], A
    // flags ----
    return sprintf(buff, "LD   [0x%04X], A", MEM_IMM16_EXTRACT(code, 0));
}

static int ld_a_ma16(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD A, [imm16]
    // flags ----
    return sprintf(buff, "LD   A, [0x%04X]", MEM_IMM16_EXTRACT(code, 0));
}

static int ld_sp_hl(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD SP, HL
    // flags ----
    return sprintf(buff, "LD   SP, HL");
}

static int add_sp_e8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // ADD SP, imm8
    // effectively: ADD SP, SP, E8
    // flags 00hc
    if( (int8_t)code[1] > 0 ) {
        return sprintf(buff, "ADD  SP, +0x%02X", code[1]);
    } else {
        return sprintf(buff, "ADD  SP, -0x%02X", -((int8_t)code[1]));
    }
}

static int ld_hl_spe8(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // LD HL, SP+imm8
    // effectively: ADD HL, SP, E8
    // flags 00hc
    if( (int8_t)code[1] > 0 ) {
        return sprintf(buff, "LD   HL, SP+0x%02X", code[1]);
    } else {
        return sprintf(buff, "LD   HL, SP-0x%02X", -((int8_t)code[1]));
    }
}

static int di(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // DI
    // flags ----
    return sprintf(buff, "DI");
}

static int ei(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    // EI
    // flags ----
    return sprintf(buff, "EI");
}

static int prefix(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr)
{
    static const char * const pfx0Decode[] = {
        "RLC ",
        "RRC ",
        "RL  ",
        "RR  ",
        "SLA ",
        "SRA ",
        "SWAP",
        "SRL ",
    };
    static const char * const pfx0FlagDecode[] = {
        "z00c",
        "z00c",
        "z00c",
        "z00c",
        "z00c",
        "z00c",
        "z000",
        "z00c",
    };

    const uint8_t pfx_inst =  code[1];
    const uint8_t pfx_block = INST_BLOCK_EXTRACT(pfx_inst);
    const uint8_t pfx_op_a  = INST_R8_A_EXTRACT(pfx_inst);
    const uint8_t pfx_bit   = INST_R8_A_EXTRACT(pfx_inst);
    const uint8_t pfx_r8    = INST_R8_B_EXTRACT(pfx_inst);

    switch( pfx_block ) {
        case INST_BLOCK0:
            return sprintf(buff, "%s %s", pfx0Decode[pfx_op_a], r8Decode[pfx_r8]);
        case INST_BLOCK1:
            return sprintf(buff, "BIT  %d, %s", pfx_bit, r8Decode[pfx_r8]);
        case INST_BLOCK2:
            return sprintf(buff, "RES  %d, %s", pfx_bit, r8Decode[pfx_r8]);
        case INST_BLOCK3:
            return sprintf(buff, "SET  %d, %s", pfx_bit, r8Decode[pfx_r8]);
        default: // impossible, squelch warning
            return 0;
    }
}

typedef int (disOp)(char * const buff, const uint8_t instruction, const uint8_t code[3], const uint16_t addr);

// Buff must be large enough to fit dissasembled instruction
// mem contains the next 3 bytes of code
// addr is the memory mapped address of the first code byte
// returns the number of bytes used
int disassemble2(char *buff, const uint8_t code[3], const int16_t addr)
{
    const uint8_t instruction = code[0];

    static disOp * const block0decode[64] = {
        nop,            ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   bitop_a,
        ld_ma16_sp,     add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   bitop_a,

        stop,           ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   bitop_a,
        jr_e8,          add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   bitop_a,

        jr_cc_e8,       ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   bitop_a,
        jr_cc_e8,       add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   bitop_a,

        jr_cc_e8,       ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   bitop_a,
        jr_cc_e8,       add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   bitop_a,
    };

    static disOp * const block3decode[64] = {
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
            return block0decode[(instruction & ~INST_BLOCK_MASK)](buff, instruction, code, addr);
            break;
        case INST_BLOCK1:
            // Mostly register to register loads
            if(INST_HALT == instruction) {
                return halt(buff, instruction, code, addr);
            } else {
                return ld_r8_r8(buff, instruction, code, addr);
            }
            break;
        case INST_BLOCK2:
            // all register based ALU operations
            return alu_r8(buff, instruction, code, addr);
            break;
        case INST_BLOCK3:
            return block3decode[(instruction & ~INST_BLOCK_MASK)](buff, instruction, code, addr);
            break;
        default: // impossible, squelch warning
            return 0;
    }
}

int instructionSize(const uint8_t instruction)
{
    static const uint8_t block0Sizes[64] = {
        //  nop,            ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   rlc_a,
            1,              3,              1,              1,          1,              1,          2,          1,
        //  ld_ma16_sp,     add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   rrc_a,
            3,              1,              1,              1,          1,              1,          2,          1,

        //  stop,           ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   rl_a,
            1,              3,              1,              1,          1,              1,          2,          1,
        //  jr_e8,          add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   rr_a,
            2,              1,              1,              1,          1,              1,          2,          1,

        //  jr_cc_e8,       ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   daa,
            2,              3,              1,              1,          1,              1,          2,          1,
        //  jr_cc_e8,       add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   cpl,
            2,              1,              1,              1,          1,              1,          2,          1,

        //  jr_cc_e8,       ld_r16_i16,     ld_mr16_a,      inc_r16,    inc_r8,         dec_r8,     ld_r8_i8,   scf,
            2,              3,              1,              1,          1,              1,          2,          1,
        //  jr_cc_e8,       add_hl_r16,     ld_a_mr16,      dec_r16,    inc_r8,         dec_r8,     ld_r8_i8,   ccf,
            2,              1,              1,              1,          1,              1,          2,          1,
    };

    static const uint8_t block3Sizes[64] = {
        //  ret_cc,         pop_r16,        jp_cc_i16,      jp_i16,     call_cc_i16,    push_r16,   alu_i8,     rst,
            1,              1,              3,              3,          3,              1,          2,          3,
        //  ret_cc,         ret,            jp_cc_i16,      prefix,     call_cc_i16,    call_i16,   alu_i8,     rst,
            1,              1,              3,              2,          3,              3,          2,          3,

        //  ret_cc,         pop_r16,        jp_cc_i16,      invalid,    call_cc_i16,    push_r16,   alu_i8,     rst,
            1,              1,              3,              1,          3,              1,          2,          3,
        //  ret_cc,         reti,           jp_cc_i16,      invalid,    call_cc_i16,    invalid,    alu_i8,     rst,
            1,              1,              3,              1,          3,              1,          2,          3,

        //  ldh_ma8_a,      pop_r16,        ldh_mc_a,       invalid,    invalid,        push_r16,   alu_i8,     rst,
            2,              1,              1,              1,          1,              1,          2,          3,
        //  add_sp_e8,      jp_hl,          ld_ma16_a,      invalid,    invalid,        invalid,    alu_i8,     rst,
            2,              1,              3,              1,          1,              1,          2,          3,

        //  ldh_a_ma8,      pop_r16,        ldh_a_mc,       di,         invalid,        push_r16,   alu_i8,     rst,
            2,              1,              1,              1,          1,              1,          2,          3,
        //  ld_hl_spe8,     ld_sp_hl,       ld_a_ma16,      ei,         invalid,        invalid,    alu_i8,     rst,
            2,              1,              3,              1,          1,              1,          2,          3,
    };

    const uint8_t block = INST_BLOCK_EXTRACT(instruction);

    switch( block ) {
        case INST_BLOCK0:
            return block0Sizes[(instruction & ~INST_BLOCK_MASK)];
        case INST_BLOCK1:
            // ld_r8_r8 and halt
            return 1;  // all self-contained, one byte
        case INST_BLOCK2:
            // all register based ALU operations
            // alu_r8
            return 1;  // all self-contained, one byte
        case INST_BLOCK3:
            return block3Sizes[(instruction & ~INST_BLOCK_MASK)];
        default: // impossible, squelch warning
            return 0;
    }
}

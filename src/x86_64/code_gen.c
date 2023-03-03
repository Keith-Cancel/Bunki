#include <stdint.h>
#include <string.h>

#define OP_CREATE(OPCODE, DIR, WORD)   ((((OPCODE) & 0x3f) << 2) | (((DIR) & 1) << 1) | ((WORD) & 1))
#define REX_CREATE(W, R, X, B)         (0x40 | (((W) & 1) << 3) | (((R) & 1) << 2) | (((X) & 1) << 1) | ((B) & 1))
#define MOD_RM_CREATE(MOD, REG, RM)    ((((MOD) & 0x3) << 6) | (((REG) & 0x7) << 3) | ((RM) & 0x7))
#define SIB_CREATE(SCALE, INDEX, BASE) (((SCALE & 0x3) << 6) | (((INDEX) & 0x7) << 3) | ((BASE) & 0x7))

#define REX_MAGIC 0x4

#define SCALE_1X 0x0
#define SCALE_2x 0x1
#define SCALE_4X 0x2
#define SCALE_8X 0x3

#define MOD_REG_INDIR  0x0
#define MOD_REG_DISP8  0x1
#define MOD_REG_DISP32 0x2
#define MOD_REG_DIR    0x3

// D_W OP Codes
#define OP_ADD      0x00
#define OP_ADDI_EAX 0x01
#define OP_AND      0x08
#define OP_ANDI_EAX 0x09
#define OP_SUB      0x0a
#define OP_SUBI_EAX 0x0b
#define OP_XOR      0x0c
#define OP_XORI_EAX 0x0d
#define OP_IMM      0x20
#define OP_MOV      0x22
// Whole byte opcodes
#define OP_PUSH 0x50
#define OP_POP  0x58
// MODRM.REG OP extensions
#define OP_EX_ADD 0x0
#define OP_EX_OR  0x1
#define OP_EX_ADC 0x2
#define OP_EX_SBB 0x3
#define OP_EX_AND 0x4
#define OP_EX_SUB 0x5
#define OP_EX_XOR 0x6
#define OP_EX_CMP 0x7

#define RAX 0x0
#define RCX 0x1
#define RDX 0x2
#define RBX 0x3
#define RSP 0x4
#define RBP 0x5
#define RSI 0x6
#define RDI 0x7
#define R8  0x8
#define R9  0x9
#define R10 0xa
#define R11 0xb
#define R12 0xc
#define R13 0xd
#define R14 0xe
#define R15 0xf

typedef union byte_types_u {
    struct {
        uint8_t B:1;
        uint8_t X:1;
        uint8_t R:1;
        uint8_t W:1;
        uint8_t magic:4;
    } rex;
    struct {
        uint8_t word:1;
        uint8_t dir:1;
        uint8_t opcode:6;
    } op;
    struct {
        uint8_t rm:3;
        uint8_t reg:3;
        uint8_t mod:2;
    } mod_rm;
    struct {
        uint8_t base:3;
        uint8_t index:3;
        uint8_t scale:2;
    } sib;
    uint8_t byte;
} byte_t;

typedef struct ins_bytes_s {
    byte_t*  prefix;
    byte_t*  start;
    unsigned len;
} insBytes_t;

typedef uint32_t uint;

// Helpers

static insBytes_t gen_rex_byte(uint8_t* out, uint W,  uint R, uint X, uint B) {
    insBytes_t ins = {
        .prefix = (byte_t*)(out),
        .start  = (byte_t*)(out),
        .len    = 0
    };
    if(W | R | X| B) {
        ins.prefix->rex.magic = REX_MAGIC;
        ins.prefix->rex.W = W;
        ins.prefix->rex.R = R;
        ins.prefix->rex.X = X;
        ins.prefix->rex.B = B;
        ins.start += 1;
        ins.len++;
    }
    return ins;
}

static insBytes_t gen_op_reg(uint8_t* out, uint width, uint dest, uint src, uint opcode) {
    uint       du    = dest >> 3;
    uint       su    = src  >> 3;
    insBytes_t ins   = gen_rex_byte(out, width, su, 0, du);
    byte_t*    start = ins.start;

    start[0].op.opcode = opcode;
    start[0].op.dir    = 0;
    start[0].op.word   = 1;

    start[1].mod_rm.mod = MOD_REG_DIR;
    start[1].mod_rm.reg = src;
    start[1].mod_rm.rm  = dest;

    ins.len += 2;
    return ins;
}

static insBytes_t gen_op_imm8(uint8_t* out, uint width, uint reg, uint opcode, uint imm8) {
    insBytes_t ins = gen_op_reg(out, width, reg, opcode, OP_IMM);
    ins.start->op.dir = 1;
    ins.start[2].byte = imm8;
    ins.len += 1;
    return ins;
}

static insBytes_t gen_op_imm32(uint8_t* out, uint width, uint reg, uint opcode, uint imm32) {
    insBytes_t ins = gen_op_reg(out, width, reg, opcode, OP_IMM);
    memcpy(ins.start + 2, &imm32, sizeof(uint));
    ins.len += 4;
    return ins;
}

static insBytes_t gen_op_indirect_disp8(uint8_t* out, uint width, uint mem, uint reg, uint opcode, uint read, uint disp8) {
    insBytes_t ins = gen_op_reg(out, width, mem, reg, opcode);
    uint mem_low = mem & 0x7;
    uint off     = mem_low == 0x4;
    ins.start[0].op.dir     = read;
    ins.start[1].mod_rm.mod = MOD_REG_DISP8;
    if(off) {
        ins.start[2].sib.scale = SCALE_1X;
        ins.start[2].sib.index = 0x4;
        ins.start[2].sib.base  = 0x4;
        ins.len += 1;
    }
    ins.start[2 + off].byte = disp8;
    ins.len += 1;
    return ins;
}

static insBytes_t gen_op_indirect_disp32(uint8_t* out, uint width, uint mem, uint reg, uint opcode, uint read, uint disp32) {
    insBytes_t ins = gen_op_indirect_disp8(out, width, mem, reg, opcode, read, disp32);
    ins.start[1].mod_rm.mod = MOD_REG_DISP32;
    memcpy(ins.prefix + ins.len - 1, &disp32, sizeof(uint));
    ins.len += 3;
    return ins;
}

static insBytes_t gen_op_indirect(uint8_t* out, uint width, uint mem, uint reg, uint opcode, uint read) {
    uint8_t mem_low = mem & 0x7;
    if(mem_low == 0x5) {
        return gen_op_indirect_disp8(out, width, mem, reg, opcode, read, 0);
    }
    insBytes_t ins = gen_op_reg(out, width, mem, reg, opcode);
    ins.start[0].op.dir     = read;
    ins.start[1].mod_rm.mod = MOD_REG_INDIR;
    if(mem_low == 0x4) {
        ins.start[2].sib.scale = SCALE_1X;
        ins.start[2].sib.index = 0x4;
        ins.start[2].sib.base  = 0x4;
        ins.len += 1;
    }
    return ins;
}

static unsigned gen_op32_eax_imm32(uint8_t* out, unsigned opcode, uint32_t imm32) {
    out[0] = OP_CREATE(opcode, 0, 1);
    memcpy(out + 1, &imm32, sizeof(uint32_t));
    return 5;
}

static unsigned gen_op64_eax_imm32(uint8_t* out, unsigned opcode, uint32_t imm32) {
    out[0] = REX_CREATE(1, 0, 0, 0);
    out[1] = OP_CREATE(opcode, 0, 1);
    memcpy(out + 2, &imm32, sizeof(uint32_t));
    return 6;
}

static unsigned gen_op_unary(uint8_t* out, unsigned reg, uint8_t opcode) {
    unsigned ru  = reg >> 3;
    unsigned len = 1 + ru;
    if(len > 1) {
        *out = REX_CREATE(0, 0, 0, ru);
        out += 1;
    }
    *out = opcode | (reg & 0x7);
    return len;
}

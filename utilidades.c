#include "utilidades.h"
#include "instrucciones.h"



InstruccionFunc operaciones[32] = {
    inst_sys, inst_jmp, inst_jp,  inst_jn,  inst_jz,  inst_jc,  inst_jv,  inst_jnp,
    inst_jnn, inst_jnz, inst_not, inst_invalida, inst_invalida, inst_invalida, inst_invalida, inst_stop,
    inst_mov, inst_add, inst_sub, inst_mul, inst_div, inst_cmp, inst_and, inst_or,
    inst_xor, inst_swap, inst_shl, inst_shr, inst_sar, inst_ldl, inst_ldh, inst_rnd
};

const char* mnemonicos[32] = {
    "SYS", "JMP", "JP",  "JN",  "JZ",  "JC",  "JV",  "JNP",
    "JNN", "JNZ", "NOT", "INV", "INV", "INV", "INV", "STOP",
    "MOV", "ADD", "SUB", "MUL", "DIV", "CMP", "AND", "OR",
    "XOR", "SWAP", "SHL", "SHR", "SAR", "LDL", "LDH", "RND"
};

const char* nom_regs[32] = {
    "IP", "OPC", "OP1", "OP2", "LAR", "MAR", "MBR", "RES", "RES", "RES",
    "EAX", "EBX", "ECX", "EDX", "EEX", "EFX", "AC", "CC", "RES", "RES",
    "RES", "RES", "RES", "RES", "RES", "RES", "CS", "DS", "RES", "RES",
    "RES", "RES"
};
#include "utilidades.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "instrucciones.h"


void leerExacto(void *dst, size_t n, FILE *arch) {
    if (fread(dst, 1, n, arch) != n) {
        printf("Error: archivo truncado o corrupto.\n");
        fclose(arch);
        exit(1);
    }
}
unsigned short leerUShortEndian(FILE *arch) {
    unsigned char b[2];
    leerExacto(b, 2, arch);
    return (unsigned short)((b[0] << 8) | b[1]);
}
short int leerShortEndian(FILE *arch) {
    return (short int)leerUShortEndian(arch);
}
int leerIntEndian(FILE *arch) {
    unsigned char b[4];
    leerExacto(b, 4, arch);
    return (int)(((unsigned int)b[0] << 24) | ((unsigned int)b[1] << 16) |
                 ((unsigned int)b[2] << 8)  |  (unsigned int)b[3]);
}

int terminaEn(const char *s, const char *ext) {
    size_t n = strlen(s), e = strlen(ext);
    return n > e && strcmp(s + n - e, ext) == 0;
}

// Devuelve el tamanio de RAM (KiB) guardado en el header del .vmi, o -1 si falla
int leerTamRamVMI(const char *ruta) {
    FILE *f = fopen(ruta, "rb");
    if (f == NULL) return -1;
    unsigned char h[8];
    size_t leidos = fread(h, 1, 8, f);
    fclose(f);
    if (leidos != 8 || memcmp(h, "VMI26", 5) != 0) return -1;
    return (h[6] << 8) | h[7];
}
InstruccionFunc operaciones[32] = {
    inst_sys, inst_jmp, inst_jp,  inst_jn,  inst_jz,  inst_jc,  inst_jv,  inst_jnp,
    inst_jnn, inst_jnz, inst_not, inst_push, inst_pop, inst_call, inst_ret, inst_stop,
    inst_mov, inst_add, inst_sub, inst_mul, inst_div, inst_cmp, inst_and, inst_or,
    inst_xor, inst_swap, inst_shl, inst_shr, inst_sar, inst_ldl, inst_ldh, inst_rnd
};

const char* mnemonicos[32] = {
    "SYS", "JMP", "JP",  "JN",  "JZ",  "JC",  "JV",  "JNP",
    "JNN", "JNZ", "NOT", "PUSH", "POP", "CALL", "RET", "STOP",
    "MOV", "ADD", "SUB", "MUL", "DIV", "CMP", "AND", "OR",
    "XOR", "SWAP", "SHL", "SHR", "SAR", "LDL", "LDH", "RND"
};

const char* nom_regs[32] = {
    "IP", "OPC", "OP1", "OP2", "LAR", "MAR", "MBR", "SP", "BP", "RES",
    "EAX", "EBX", "ECX", "EDX", "EEX", "EFX", "AC", "CC", "RES", "RES",
    "RES", "RES", "RES", "RES", "RES", "RES", "CS", "DS", "ES", "SS",
    "KS", "PS"
};


#include "instrucciones.h"
#include "utilidades.h"
#include <stdio.h>

void imprimirOperando(int tipo, int valor, const char* nom_regs[]) {
    if (tipo == 1) {
        printf("%s", nom_regs[valor]);
        return;
    }
    if (tipo == 2) {
        printf("%d", valor);
        return;
    }
    if (tipo == 3) {
        short offset = (short)(valor >> 8);
        int reg = valor & 0x1F;

        if (offset == 0) {
            printf("[%s]", nom_regs[reg]);
            return;
        }
        if (offset > 0) {
            printf("[%s+%d]", nom_regs[reg], offset);
            return;
        }
        if (offset < 0) {
            printf("[%s%d]", nom_regs[reg], offset);
            return;
        }
    }
}

void imprimirDisassembler(int pc, int opcode, int tipoA, int tipoB, int valorA, int valorB, char memoriaPrincipal[]) {
    printf("[%04X] ", pc);

    int tam_instruccion = 1 + tipoA + tipoB;
    for(int j = 0; j < tam_instruccion; j++) {
        printf("%02X ", (unsigned char)memoriaPrincipal[pc + j]);
    }

    if (tam_instruccion < 6) printf("\t");

    printf("| %s ", mnemonicos[opcode]);

    if (tipoA != 0) imprimirOperando(tipoA, valorA, nom_regs);

    if (tipoB != 0) {
        printf(", ");
        imprimirOperando(tipoB, valorB, nom_regs);
    }
    printf("\n");
}
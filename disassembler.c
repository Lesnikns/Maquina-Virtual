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

void volcadoDisassembler(char memoriaPrincipal[], short int tamCod) {
    printf("--- INICIO DISASSEMBLER ---\n");
    int pc = 0;

    while (pc < tamCod) {
        unsigned char primer_byte = memoriaPrincipal[pc];
        int opcode, tipoA, tipoB;

        if ((primer_byte & 0x30) == 0) {
            opcode = primer_byte & 0x1F;
            tipoA = (primer_byte >> 6) & 0x03;
            tipoB = 0;
        } else {
            opcode = (primer_byte & 0x0F) | 0x10;
            tipoA = (primer_byte >> 4) & 0x03;
            tipoB = (primer_byte >> 6) & 0x03;
        }

        int valorA = 0, valorB = 0;
        int offset_actual = pc + 1;

        if (tipoB == 1) {
            valorB = memoriaPrincipal[offset_actual] & 0xFF;
            offset_actual += 1;
        } else if (tipoB == 2) {
            short inm = ((memoriaPrincipal[offset_actual]&0xFF)<<8)|(memoriaPrincipal[offset_actual+1]&0xFF);
            valorB = (int)inm;
            offset_actual += 2;
        } else if (tipoB == 3) {
            valorB = ((memoriaPrincipal[offset_actual]&0xFF)<<16)|((memoriaPrincipal[offset_actual+1]&0xFF)<<8)|(memoriaPrincipal[offset_actual+2]&0xFF);
            offset_actual += 3;
        }

        if (tipoA == 1) {
            valorA = memoriaPrincipal[offset_actual] & 0xFF;
            offset_actual += 1;
        } else if (tipoA == 2) {
            short inm = ((memoriaPrincipal[offset_actual]&0xFF)<<8)|(memoriaPrincipal[offset_actual+1]&0xFF);
            valorA = (int)inm;
            offset_actual += 2;
        } else if (tipoA == 3) {
            valorA = ((memoriaPrincipal[offset_actual]&0xFF)<<16)|((memoriaPrincipal[offset_actual+1]&0xFF)<<8)|(memoriaPrincipal[offset_actual+2]&0xFF);
            offset_actual += 3;
        }

        imprimirDisassembler(pc, opcode, tipoA, tipoB, valorA, valorB, memoriaPrincipal);

        // Avanzamos el PC para leer la siguiente instrucción
        pc += (1 + tipoA + tipoB);
    }
    printf("--- FIN DISASSEMBLER ---\n\n");
}
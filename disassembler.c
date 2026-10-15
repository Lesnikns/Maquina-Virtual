#include "utilidades.h"
#include <stdio.h>
#include <string.h>

void imprimirOperando(int tipo, int valor_crudo, const char* nom_regs[]) {
    if (tipo == 1) {
        int reg = valor_crudo & 0x1F;
        int sector = (valor_crudo >> 5) & 0x03;
        const char* base_name = nom_regs[reg];

        if (sector == 0) {
            printf("%s", base_name);
        } else {
            // Generar el pseudonimo dinamicamente
            if (strlen(base_name) >= 3 && base_name[0] == 'E' && base_name[2] == 'X') {
                if (sector == 1) printf("%cL", base_name[1]);      // LOW
                else if (sector == 2) printf("%cH", base_name[1]); // HIGH
                else if (sector == 3) printf("%cX", base_name[1]); // 2 BYTE
            } else {
                printf("%s", base_name); // Fallback si no es un registro E_X
            }
        }
        return;
    }
    if (tipo == 2) {
        printf("%d", (short)(valor_crudo & 0xFFFF));
        return;
    }
    if (tipo == 3) {
        short offset = (short)(valor_crudo >> 8);
        int reg = valor_crudo & 0x1F;
        int modificador = (valor_crudo >> 5) & 0x07;

        // Decodificar el modificador de tamaño de memoria
        const char* prefijo = "";
        if (modificador == 1) prefijo = "b";
        else if (modificador == 2) prefijo = "w";
        else if (modificador == 4) prefijo = "l";

        if (offset == 0) {
            printf("%s[%s]", prefijo, nom_regs[reg]);
        } else if (offset > 0) {
            printf("%s[%s+%d]", prefijo, nom_regs[reg], offset);
        } else {
            printf("%s[%s%d]", prefijo, nom_regs[reg], offset);
        }
        return;
    }
}

void imprimirDisassembler(int pc, int opcode, int tipoA, int tipoB, int valorA, int valorB, char memoriaPrincipal[], int es_entry_point) {
    // Indicador de entry point
    if (es_entry_point)
        printf("> ");
    else
        printf("  ");

    printf("[%04X] ", pc);

    int tam_instruccion = 1 + tipoA + tipoB;
    for(int j = 0; j < tam_instruccion; j++) {
        printf("%02X ", (unsigned char)memoriaPrincipal[pc + j]);
    }

    // Alinear el texto si la instruccion tiene menos de 6 bytes
    if (tam_instruccion < 6) {
        int espacios = (6 - tam_instruccion) * 3;
        for(int k=0; k<espacios; k++) printf(" ");
    }

    printf("| %s ", mnemonicos[opcode]);

    if (tipoA != 0) imprimirOperando(tipoA, valorA, nom_regs);

    if (tipoB != 0) {
        printf(", ");
        imprimirOperando(tipoB, valorB, nom_regs);
    }
    printf("\n");
}

void volcadoDisassembler(char memoriaPrincipal[], short int tablaSegmentos[8][2], int registros[32], int offsetEP) {
    printf("--- INICIO DISASSEMBLER ---\n");

    // Mostrar KS
    int idx_ks = registros[KS] >> 16;
    if (idx_ks >= 0 && idx_ks < 8 && tablaSegmentos[idx_ks][1] > 0) {
        int base_ks = tablaSegmentos[idx_ks][0];
        int limite_ks = base_ks + tablaSegmentos[idx_ks][1];
        int pc = base_ks;

        while (pc < limite_ks) {
            int len = 0;
            // Buscar longitud de la cadena hasta encontrar el '\0'
            while (pc + len < limite_ks && memoriaPrincipal[pc + len] != '\0') {
                len++;
            }
            len++; // Incluir el '\0'

            printf("  [%04X] ", pc);

            // Logica de visualizacion de bytes hexadecimales
            if (len <= 7) {
                for (int j = 0; j < len; j++) {
                    printf("%02X ", (unsigned char)memoriaPrincipal[pc + j]);
                }
                int espacios = (7 - len) * 3;
                for (int j = 0; j < espacios; j++) printf(" ");
                printf("  | \"");
            } else {
                for (int j = 0; j < 6; j++) {
                    printf("%02X ", (unsigned char)memoriaPrincipal[pc + j]);
                }
                printf(".. | \"");
            }

            // Mostrar texto con caracteres no imprimibles como '.'
            for (int j = 0; j < len - 1; j++) {
                unsigned char c = memoriaPrincipal[pc + j];
                if (c >= 32 && c <= 126) {
                    printf("%c", c);
                } else {
                    printf(".");
                }
            }
            printf("\"\n");

            pc += len;
        }
    }

    // 2. Mostrar CODE SEGMENT (CS)
    int idx_cs = registros[CS] >> 16;
    if (idx_cs >= 0 && idx_cs < 8 && tablaSegmentos[idx_cs][1] > 0) {
        int base_cs = tablaSegmentos[idx_cs][0];
        int limite_cs = base_cs + tablaSegmentos[idx_cs][1];
        int pc = base_cs;

        int entry_point_fisico = base_cs + offsetEP;

        while (pc < limite_cs) {
            int es_ep = (pc == entry_point_fisico);

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

            imprimirDisassembler(pc, opcode, tipoA, tipoB, valorA, valorB, memoriaPrincipal, es_ep);

            pc += (1 + tipoA + tipoB);
        }
    }

    printf("--- FIN DISASSEMBLER ---\n\n");
}
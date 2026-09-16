#include "funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int saltoValido(int destino, short int seg[][2]) {
    int limiteCodeSeg = seg[0][1];

    if (destino < 0 || destino >= limiteCodeSeg) {
        return 0;
    }
    return 1;
}
void inst_sys (int reg[], char mem[], short int seg[][2]);
void inst_jmp (int reg[], char mem[], short int seg[][2]);
void inst_jp  (int reg[], char mem[], short int seg[][2]);
void inst_jn  (int reg[], char mem[], short int seg[][2]);
void inst_jz  (int reg[], char mem[], short int seg[][2]);
void inst_jc  (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg, mem, seg);
    int carry = (reg[CC] >> 29) & 1;
    if (carry) {
        if (saltoValido(opA, seg))
            reg[IP] = opA;
        else {
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}
void inst_jv  (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int overflow = (reg[CC] >> 28) & 1;
    if (overflow) {
        if (saltoValido(opA, seg))
            reg[IP] = opA;
        else {
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}
void inst_jnp (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int negativo = (reg[CC] >> 31) & 1;
    int zero = (reg[CC] >> 30) & 1;

    if (zero || negativo) {
        if (saltoValido(opA, seg))
            reg[IP] = opA;
        else {
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}
void inst_jnn (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int negativo = (reg[CC] >> 31) & 1;

    if (!negativo) {
        if (saltoValido(opA, seg))
            reg[IP] = opA;
        else {
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}
void inst_jnz (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int zero = (reg[CC] >> 30) & 1;

    if (!zero) {
        if (saltoValido(opA, seg))
            reg[IP] = opA;
        else {
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}
void inst_not (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int resultado = ~opA;

    set_valor(reg[OP1], resultado, reg, mem, seg);
    actualizarCC(reg,resultado,0,0);
}
void inst_invalida(int reg[], char mem[], short int seg[][2]){
    printf("INSTRUCCION INVALIDA");
    exit(1);
}
void inst_stop(int reg[], char mem[], short int seg[][2]);
void inst_mov (int reg[], char mem[], short int seg[][2]);
void inst_add (int reg[], char mem[], short int seg[][2]);
void inst_sub (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);
    int resta = opA - opB;
    int carry = 0;
    int overflow = 0;
    if ((unsigned int)opB > (unsigned int)opA)
        carry = 1;
    if ((opA > 0 && opB < 0 && resta < 0) ||
        (opA < 0 && opB > 0 && resta > 0)) {
        overflow = 1;
        }

    set_valor(reg[OP1], resta, reg, mem, seg);
    actualizarCC(reg,resta,carry,overflow);
}
void inst_mul (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);
    int carry = 0;
    int overflow = 0;
    long long producto64 = opA * opB;
    int producto32 = (int)producto64;
    if (producto64 != (long long)producto32) {
        overflow = 1;
    }
    actualizarCC(reg,producto32,carry,overflow);
    set_valor(reg[OP1], producto32, reg, mem, seg);
}
void inst_div (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);
    int cociente;
    int resto;
    if (opB == 0) {
        printf("IMPOSIBLE DIVIDIR POR CERO");
        exit(1);
    }
    else {
        cociente = opA / opB;
        resto = opA % opB;
        set_valor(reg[OP1], cociente, reg, mem,seg);
        reg[AC] = resto;
        actualizarCC(reg,cociente,0,0 );
    }


}
void inst_cmp (int reg[], char mem[], short int seg[][2]);
void inst_and (int reg[], char mem[], short int seg[][2]);
void inst_or  (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);

    int resultado = opA | opB;
    set_valor(reg[OP1], resultado, reg, mem,seg);
    actualizarCC(reg,resultado,0,0);

}
void inst_xor (int reg[], char mem[], short int seg[][2]);
void inst_swap(int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);
    set_valor(reg[OP1], opB, reg, mem,seg);
    set_valor(reg[OP2], opA,reg, mem, seg);

    actualizarCC(reg, opB, 0,0);
}
void inst_shl (int reg[], char mem[], short int seg[][2]);
void inst_shr (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);
    int carry = 0;
    int shift = opB & 0x1F; // evito que el valor de desplacamiento supere los 32 bits para que no rompa el programa
    unsigned int resultado = (unsigned int)opA >> shift;
    if (shift > 0)
        carry = (opA >> (shift - 1)) & 1; // aislo el bit que se cayo segun el desplamiento para calcular el carry
    set_valor(reg[OP1], resultado, reg, mem,seg);
    actualizarCC(reg, resultado,carry,0);

}
void inst_sar (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);
    int carry = 0;
    int shift = opB & 0x1F; // evito que el valor de desplazamiento supere los 32 bits para que no rompa el programa
    int resultado = opA >> shift; // sigue la misma logica que shr pero como propaga signo va int directo

    if (shift > 0)
        carry = (opA >> (shift - 1)) & 1; // aislo el bit que se cayo segun el desplamiento para calcular el carry

    set_valor(reg[OP1], resultado, reg, mem,seg);
    actualizarCC(reg, resultado,carry,0);
}
void inst_ldl (int reg[], char mem[], short int seg[][2]);
void inst_ldh (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);

    int parteAlta = (opB & 0xFFFF) << 16;
    int parteBaja = (opA & 0xFFFF);

    int resultado = parteAlta | parteBaja;

    set_valor(reg[OP1], resultado, reg, mem,seg);
}
void inst_rnd (int reg[], char mem[], short int seg[][2]);
void set_valor(int operando_empaquetado, int valor_a_guardar, int registros[], char memoriaPrincipal[], short int tablaSegmentos[8][2]){

    int tipo = (unsigned int)operando_empaquetado >> 24;
    int valor_crudo = operando_empaquetado & 0x00FFFFFF;

    if (tipo == 1) {
        int reg = valor_crudo & 0x1F;
        registros[reg] = valor_a_guardar;
        return;
    }

    if (tipo == 3) {
        short offset = (short)(valor_crudo >> 8);
        int reg = valor_crudo & 0x1F;
        int puntero_logico = registros[reg];
        int segmento = (puntero_logico >> 16) & 0xFFFF;
        int offset_base = puntero_logico & 0xFFFF;
        int dir_fisica = tablaSegmentos[segmento][0] + offset_base + offset;
        memoriaPrincipal[dir_fisica]     = (valor_a_guardar >> 24) & 0xFF;
        memoriaPrincipal[dir_fisica + 1] = (valor_a_guardar >> 16) & 0xFF;
        memoriaPrincipal[dir_fisica + 2] = (valor_a_guardar >> 8) & 0xFF;
        memoriaPrincipal[dir_fisica + 3] = valor_a_guardar & 0xFF;
        return;
    }

};
int get_valor(int operando_empaquetado, int registros[], char memoriaPrincipal[], short int tablaSegmentos[8][2]) {
    int tipo = (unsigned int)operando_empaquetado >> 24;
    int valor_crudo = operando_empaquetado & 0x00FFFFFF;

    if (tipo == 0) return 0; // ningun operando

    if (tipo == 1) { // registro
        int reg = valor_crudo & 0x1F;
        return registros[reg];
    }

    if (tipo == 2) { // inmediato
        short inmediato = (short)(valor_crudo & 0xFFFF);
        return (int)inmediato;
    }

    if (tipo == 3) { // memoria
        short offset = (short)(valor_crudo >> 8);
        int reg = valor_crudo & 0x1F;

        int puntero_logico = registros[reg];
        int segmento = (puntero_logico >> 16) & 0xFFFF;
        int offset_base = puntero_logico & 0xFFFF;

        int dir_fisica = tablaSegmentos[segmento][0] + offset_base + offset;


        int dato = ((memoriaPrincipal[dir_fisica] & 0xFF) << 24) |
                   ((memoriaPrincipal[dir_fisica + 1] & 0xFF) << 16) |
                   ((memoriaPrincipal[dir_fisica + 2] & 0xFF) << 8) |
                   (memoriaPrincipal[dir_fisica + 3] & 0xFF);

        return dato;
    }

    return 0;
}
void actualizarCC(int registros[], int resultado, int carry, int overflow) {

    int n = (resultado < 0) ? 1 : 0;
    int z = (resultado == 0) ? 1 : 0;


    registros[CC] &= 0x0FFFFFFF;

    registros[CC] |= ((unsigned int)n << 31);
    registros[CC] |= (z << 30);
    registros[CC] |= (carry << 29);
    registros[CC] |= (overflow << 28);
}

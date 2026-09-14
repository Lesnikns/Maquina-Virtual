#include "funciones.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void inst_sys (int reg[], char mem[], short int seg[][2]);
void inst_jmp (int reg[], char mem[], short int seg[][2]);
void inst_jp  (int reg[], char mem[], short int seg[][2]);
void inst_jn  (int reg[], char mem[], short int seg[][2]);
void inst_jz  (int reg[], char mem[], short int seg[][2]);
void inst_jc  (int reg[], char mem[], short int seg[][2]);
void inst_jv  (int reg[], char mem[], short int seg[][2]);
void inst_jnp (int reg[], char mem[], short int seg[][2]);
void inst_jnn (int reg[], char mem[], short int seg[][2]);
void inst_jnz (int reg[], char mem[], short int seg[][2]);
void inst_not (int reg[], char mem[], short int seg[][2]);
void inst_invalida(int reg[], char mem[], short int seg[][2]){
    printf("INSTRUCCION INVALIDA");
}
void inst_stop(int reg[], char mem[], short int seg[][2]);
void inst_mov (int reg[], char mem[], short int seg[][2]);
void inst_add (int reg[], char mem[], short int seg[][2]);
void inst_sub (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[2], reg,mem, seg);
    int opB = get_valor(reg[3], reg,mem, seg);
    int resta = opA - opB;
    int carry = 0;
    int overflow = 0;
    if (opB > opA)
        carry = 1;
    if ((opA > 0 && opB < 0 && resta < 0) ||
        (opA < 0 && opB > 0 && resta > 0)) {
        overflow = 1;
        }

    set_valor(opA, resta, reg, mem, seg);
    actualizarCC(reg,opA,carry,overflow);
}
void inst_mul (int reg[], char mem[], short int seg[][2]);
void inst_div (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[2], reg,mem, seg);
    int opB = get_valor(reg[3], reg,mem, seg);
    int cociente;
    int resto;
    if (opB == 0) {
        printf("IMPOSIBLE DIVIDIR POR CERO");
        exit(1);
    }
    else {
        cociente = opA / opB;
        resto = opA % opB;
        set_valor(opA, cociente, reg, mem,seg);
        reg[16] = resto;
        actualizarCC(reg,cociente,0,0 );
    }


}
void inst_cmp (int reg[], char mem[], short int seg[][2]);
void inst_and (int reg[], char mem[], short int seg[][2]);
void inst_or  (int reg[], char mem[], short int seg[][2]);
void inst_xor (int reg[], char mem[], short int seg[][2]);
void inst_swap(int reg[], char mem[], short int seg[][2]);
void inst_shl (int reg[], char mem[], short int seg[][2]);
void inst_shr (int reg[], char mem[], short int seg[][2]);
void inst_sar (int reg[], char mem[], short int seg[][2]);
void inst_ldl (int reg[], char mem[], short int seg[][2]);
void inst_ldh (int reg[], char mem[], short int seg[][2]);
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


    registros[17] &= 0x0FFFFFFF;

    registros[17] |= ((unsigned int)n << 31);
    registros[17] |= (z << 30);
    registros[17] |= (carry << 29);
    registros[17] |= (overflow << 28);
}
#include "instrucciones.h"
#include "utilidades.h"
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
void inst_sys (int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[OP1],reg,mem,seg);
    int cantByte = reg[ECX] >> 16; //cantidad de bytes a leer/escribir
    int cantValores = reg[ECX] & 0xFFFF; //cantidad de valores a leer/escribir
    int segmento    = (reg[EDX] >> 16) & 0xFFFF;
    int offset_base = reg[EDX] & 0xFFFF;
    int edx = seg[segmento][IP] + offset_base; // direccion fisica real para leer/escribir datos

    if(op_a == 1){ //para lectura
        for(int i=0;i<cantValores;i++){
            int valor_a_guardar = 0;
            if((reg[EAX] & 0xF0) == 0x10){ //lee entrada en formato binario
                char* cad_binaria = (char *)malloc(sizeof(char)*cantByte*8+1); //+ 1 para el \0
                scanf("%s", cad_binaria);
                for(int j=0;j<cantByte*8;j++){ //leo bit a bit, y voy armando el valor a guardar
                    valor_a_guardar = valor_a_guardar << 1;
                    if(cad_binaria[j] == '1')
                        valor_a_guardar += 1;
                }
                free(cad_binaria);
            }
            else
                switch(reg[EAX] & 0xF){
                    case 0x0: //imprime en decimal
                        scanf("%d", &valor_a_guardar);
                        break;
                    case 0x2: {//caracter
                        char c;
                        scanf(" %c", &c); //espacio antes de %c para ignorar
                        valor_a_guardar = (int)c;
                    };break;
                    case 0x4: //octal
                        scanf("%o", &valor_a_guardar);
                        break;
                    case 0x8: //hexadecimal
                        scanf("%X", &valor_a_guardar);
                        break;
                }
            
            for(int j=0;j<cantByte;j++)
                mem[edx+i*cantByte+j] = (valor_a_guardar >> (8*(cantByte-1-j))) & 0xFF; //comienza en cantbyte-1 para leer el primer byte mas significativo, y luego va bajando
        }
    }
    else
        if(op_a == 2){ //para escritura
            for(int i=0;i<cantValores;i++){
                int valor_a_escribir = mem[edx+i*cantByte] & 0xFF; //no usar reg 13 para edx sino usar get_valor para obtener direccion fisica??
                for(int j=1; j < cantByte; j++){ //termino de concatenar los bytes en un solo valor
                    valor_a_escribir = valor_a_escribir << 8;
                    valor_a_escribir += mem[edx+i*cantByte+j] & 0xFF;
                } 

                if(((reg[EAX] & 0xF0) == 0x10)){ //imprime en binario
                    char *s = (char *)malloc(sizeof(valor_a_escribir)*8+1);
                    devuelveNotacionBinaria(valor_a_escribir,s);
                    printf("%s\n", s);
                    free(s);
                }
                else {
                    switch(reg[EAX] & 0xF){
                        case 0x0: //imprime en decimal
                            printf("%d\n", valor_a_escribir);
                            break;
                        case 0x2: //caracter
                            printf("%c\n", valor_a_escribir);
                            break;
                        case 0x4: //octal
                            printf("%o\n", valor_a_escribir);
                            break;
                        case 0x8: //hexadecimal
                            printf("%X\n", valor_a_escribir);
                            break;
                    }
                }
            }
        }
}
void inst_jmp (int reg[], char mem[], short int seg[][2]) {
    int op = get_valor(reg[OP1],reg,mem,seg);
    if (saltoValido(op, seg))
        reg[IP] = op; //carga en ip el valor de op
    else {
        printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
        exit(1);
    }
}
void inst_jp  (int reg[], char mem[], short int seg[][2]) {
    int n = (reg[CC] >> 31) & 0x1; //bit de signo negativo
    int z = (reg[CC] >> 30) & 0x1; //bit de cero
    
    if(n == 0 && z == 0){ //bit de signo negativo es 0 y bit de cero es 0, entonces es positivo
        int op = get_valor(reg[OP1],reg,mem,seg);
        if (saltoValido(op, seg))
            reg[IP] = op; //carga en ip el valor de op
        else{
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }

    }
}
void inst_jn  (int reg[], char mem[], short int seg[][2]) {
    int n = (reg[CC] >> 31) & 0x1; //bit de signo negativo

    if(n == 1){ //si el bit de signo negativo es 1, entonces es negativo
        int op = get_valor(reg[OP1],reg,mem,seg);
        if (saltoValido(op, seg))
            reg[IP] = op; //carga en ip el valor de op
        else {
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}
void inst_jz  (int reg[], char mem[], short int seg[][2]){
    int z = (reg[CC] >> 30) & 0x1; //bit de cero

    if(z == 1){ //si el bit de cero es 1, entonces el numero es cero
        int op = get_valor(reg[2],reg,mem,seg);
        if (saltoValido(op, seg))
            reg[IP] = op; //carga en ip el valor de op
        else {
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}
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
    printf("INSTRUCCION INVALIDA\n");
    exit(1);
}
void inst_stop(int reg[], char mem[], short int seg[][2]) {
    reg[IP] = -1; //carga en ip, -1 para indicar que termino la ejecucion del programa
}
void inst_mov (int reg[], char mem[], short int seg[][2]) {
    //int op_a = get_valor(reg[OP1],reg,mem,seg);
    int op_b = get_valor(reg[OP2],reg,mem,seg);

    set_valor(reg[OP1], op_b, reg, mem, seg); //carga en A el valor de B
    actualizarCC(reg, op_b, 0, 0); //bien?
}
void inst_add(int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[OP1], reg, mem, seg);
    int op_b = get_valor(reg[OP2], reg, mem, seg);

    long long real_signed = (long long)op_a + (long long)op_b;
    unsigned long long real_unsigned = (unsigned long long)(unsigned int)op_a + (unsigned long long)(unsigned int)op_b;

    int res32_signed = (int)real_signed;
    unsigned int res32_unsigned = (unsigned int)real_unsigned;

    int overflow = (real_signed != res32_signed) ? 1 : 0;
    int carry = (real_unsigned != res32_unsigned) ? 1 : 0;

    set_valor(reg[OP1], res32_signed, reg, mem, seg);
    actualizarCC(reg, res32_signed, carry, overflow);
}

void inst_sub(int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg, mem, seg);
    int opB = get_valor(reg[OP2], reg, mem, seg);

    long long real_signed = (long long)opA - (long long)opB;
    unsigned long long real_unsigned = (unsigned long long)(unsigned int)opA - (unsigned long long)(unsigned int)opB;

    int res32_signed = (int)real_signed;
    unsigned int res32_unsigned = (unsigned int)real_unsigned;

    int overflow = (real_signed != res32_signed) ? 1 : 0;
    int carry = (real_unsigned != res32_unsigned) ? 1 : 0;

    set_valor(reg[OP1], res32_signed, reg, mem, seg);
    actualizarCC(reg, res32_signed, carry, overflow);
}

void inst_mul(int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg, mem, seg);
    int opB = get_valor(reg[OP2], reg, mem, seg);

    long long real_signed = (long long)opA * (long long)opB;
    unsigned long long real_unsigned = (unsigned long long)(unsigned int)opA * (unsigned long long)(unsigned int)opB;

    int res32_signed = (int)real_signed;
    unsigned int res32_unsigned = (unsigned int)real_unsigned;

    int overflow = (real_signed != res32_signed) ? 1 : 0;
    int carry = (real_unsigned != res32_unsigned) ? 1 : 0;

    set_valor(reg[OP1], res32_signed, reg, mem, seg);
    actualizarCC(reg, res32_signed, carry, overflow);
}

void inst_div(int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg, mem, seg);
    int opB = get_valor(reg[OP2], reg, mem, seg);
    int cociente;
    int resto;

    if (opB == 0) {
        printf("IMPOSIBLE DIVIDIR POR CERO\n");
        exit(1);
    }

    if (opA == 0x80000000 && opB == -1) {
        cociente = 0x80000000;
        resto = 0;
    } else {
        cociente = opA / opB;
        resto = opA % opB;
    }

    set_valor(reg[OP1], cociente, reg, mem, seg);
    reg[AC] = resto;
    actualizarCC(reg, cociente, 0, 0);
}

void inst_cmp(int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[OP1], reg, mem, seg);
    int op_b = get_valor(reg[OP2], reg, mem, seg);

    long long real_signed = (long long)op_a - (long long)op_b;
    unsigned long long real_unsigned = (unsigned long long)(unsigned int)op_a - (unsigned long long)(unsigned int)op_b;

    int res32_signed = (int)real_signed;
    unsigned int res32_unsigned = (unsigned int)real_unsigned;

    int overflow = (real_signed != res32_signed) ? 1 : 0;
    int carry = (real_unsigned != res32_unsigned) ? 1 : 0;

    actualizarCC(reg, res32_signed, carry, overflow);
}
void inst_and (int reg[], char mem[], short int seg[][2]) {
    
    int op_a = get_valor(reg[OP1],reg,mem,seg);
    int op_b = get_valor(reg[OP2],reg,mem,seg);

    int resultado = op_a & op_b;
    set_valor(reg[OP1], resultado, reg, mem, seg);

    actualizarCC(reg, resultado, 0, 0);
}
void inst_or  (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);

    int resultado = opA | opB;
    set_valor(reg[OP1], resultado, reg, mem,seg);
    actualizarCC(reg,resultado,0,0);

}
void inst_xor (int reg[], char mem[], short int seg[][2]) {
    
    int op_a = get_valor(reg[OP1],reg,mem,seg);
    int op_b = get_valor(reg[OP2],reg,mem,seg);

    int resultado = op_a ^ op_b;
    set_valor(reg[OP1], resultado, reg, mem, seg);

    actualizarCC(reg, resultado, 0, 0);
}
void inst_swap(int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);
    set_valor(reg[OP1], opB, reg, mem,seg);
    set_valor(reg[OP2], opA,reg, mem, seg);

    actualizarCC(reg, opB, 0,0);
}
void inst_shl (int reg[], char mem[], short int seg[][2]) {
    
    int op_a = get_valor(reg[OP1],reg,mem,seg);
    int op_b = get_valor(reg[OP2],reg,mem,seg);

    int resultado = op_a << op_b;
    set_valor(reg[OP1], resultado, reg, mem, seg);

    int carry = (op_a & (1 << (32 - op_b))) ? 1 : 0; 
    int overflow = ((op_a > 0 && resultado < 0) || (op_a < 0 && resultado > 0)) ? 1 : 0;
    actualizarCC(reg, resultado, carry, overflow);
}
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
void inst_ldl (int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[OP1],reg,mem,seg);
    int op_b = get_valor(reg[OP2],reg,mem,seg);

    //carga los 2 bytes menos significativos del A, con los 2 bytes menos significativos de B
    int resultado = (op_a & 0xFFFF0000) | (op_b & 0x0000FFFF); 
    set_valor(reg[OP1], resultado, reg, mem, seg);
}
void inst_ldh (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);

    int parteAlta = (opB & 0xFFFF) << 16;
    int parteBaja = (opA & 0xFFFF);

    int resultado = parteAlta | parteBaja;

    set_valor(reg[OP1], resultado, reg, mem,seg);
}
void inst_rnd (int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[OP1],reg,mem,seg);
    int op_b = get_valor(reg[OP2],reg,mem,seg);

    //genera numero aleatorio entre 0 y op_b, y lo guarda en A
    int resultado = rand() % (op_b + 1);
    set_valor(reg[OP1], resultado, reg, mem, seg);
}

void devuelveNotacionBinaria(int nro,char *s){ //funcion auxiliar para imprimir numeros en binario, recibe un numero y un string donde se guardara la notacion binaria
    char* aux = s,car;int longBits,i;
    longBits = sizeof(nro)*8;
    if (nro<0)
        car='1';
    else
        car='0';
    for(i=0;i<longBits;i++){
        *s=car;
        s++;
    }
    *s='\0';
    s=s-1;
    i--;
    while(nro>0 || (nro<0 && i>0)){
        if(nro & 0b1)
            *s='1';
        else
            *s='0';
        s=s-1;
        nro=nro>>1;
        if(nro<0)
            i--;
    }
    s = aux;
}
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


        if (segmento < 0 || segmento > 7 || tablaSegmentos[segmento][0] == -1) {
            printf("Fallo de segmento\n");
            exit(1);
        }
        int offset_final = offset_base + offset;
        if (offset_final < 0 || (offset_final + 3) >= tablaSegmentos[segmento][1]) {
            printf("Fallo de segmento\n");
            exit(1);
        }
        int dir_fisica = tablaSegmentos[segmento][0] + offset_final; // Usamos el offset_final acá

        memoriaPrincipal[dir_fisica]     = (valor_a_guardar >> 24) & 0xFF;
        memoriaPrincipal[dir_fisica + 1] = (valor_a_guardar >> 16) & 0xFF;
        memoriaPrincipal[dir_fisica + 2] = (valor_a_guardar >> 8) & 0xFF;
        memoriaPrincipal[dir_fisica + 3] = valor_a_guardar & 0xFF;
        return;
    }
}
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

        if (segmento < 0 || segmento > 7 || tablaSegmentos[segmento][0] == -1) {
            printf("Fallo de segmento\n");
            exit(1);
        }

        int offset_final = offset_base + offset;
        // Verificamos que los 4 bytes que vamos a leer entren en el segmento
        if (offset_final < 0 || (offset_final + 3) >= tablaSegmentos[segmento][1]) {
            printf("Fallo de segmento\n");
            exit(1);
        }

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
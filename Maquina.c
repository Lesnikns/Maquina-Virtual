#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "funciones.h"
#define ram 16384

typedef void (*InstruccionFunc)(int registros[], char memoriaPrincipal[], short int tablaSegmentos[8][2]);

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

void set_valor(int operando_empaquetado, int valor_a_guardar, int registros[], char memoriaPrincipal[], short int tablaSegmentos[8][2]) {
    
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

void imprimirOperando(int tipo, int valor, const char* nom_regs[]) { // esta funcion es para dar el formato segun el tipo de opa y opb

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

void ejecutarProceso(char memoriaPrincipal[ram], int registros[32], short int tablaSegmentos[8][2], int flag_d) {
    while (1) {
        int segmento_ip = (registros[0] >> 16) & 0xFFFF;
        int offset_ip = registros[0] & 0xFFFF;

        if (segmento_ip < 0 || segmento_ip > 7 || tablaSegmentos[segmento_ip][0] == -1) {
             printf("FALLO DE SEGMENTO: Segmento invalido.\n");
             exit(1);
        }

        int pc = tablaSegmentos[segmento_ip][0] + offset_ip;

        if (offset_ip >= tablaSegmentos[segmento_ip][1]) { 
             printf("FALLO DE SEGMENTO: Acceso fuera de limites.\n");
             exit(1); 
        }
        
        if (registros[0] == -1) { // stop
             break; 
        }

        unsigned char primer_byte = memoriaPrincipal[pc];
        int opcode, tipoA, tipoB;

        if ((primer_byte & 0x30) == 0) {  // si los bits 5 y 4 son 00, es instruccion de 1 o 0 operandos
            opcode = primer_byte & 0x1F;       
            tipoA = (primer_byte >> 6) & 0x03; 
            tipoB = 0;                         
        } 
        else {
            opcode = (primer_byte & 0x0F) | 0x10; 
            tipoA = (primer_byte >> 4) & 0x03;    
            tipoB = (primer_byte >> 6) & 0x03;    
        }

        registros[1] = opcode;// pasamos a opc el codigo de operacion
        registros[0] += (1 + tipoA + tipoB); // movemos ip
        
        int valorA = 0, valorB = 0;
        int offset_actual = pc + 1; 
        
        if (tipoA == 1) { // registro 01
            valorA = memoriaPrincipal[offset_actual] & 0xFF; 
            offset_actual += 1;
        } 
        else if (tipoA == 2) { // inmediato 02
            short inmediato = ((memoriaPrincipal[offset_actual] & 0xFF) << 8) | (memoriaPrincipal[offset_actual + 1] & 0xFF);
            valorA = (int)inmediato; 
            offset_actual += 2;
        } 
        else if (tipoA == 3) { // memoria 03
            valorA = ((memoriaPrincipal[offset_actual] & 0xFF) << 16) | 
                     ((memoriaPrincipal[offset_actual + 1] & 0xFF) << 8) | 
                     (memoriaPrincipal[offset_actual + 2] & 0xFF);
            offset_actual += 3;
        }
            
        if (tipoB == 1) { 
            valorB = memoriaPrincipal[offset_actual] & 0xFF;
            offset_actual++;
        } 
        else if (tipoB == 2) { 
            short inmediato = ((memoriaPrincipal[offset_actual] & 0xFF) << 8) | (memoriaPrincipal[offset_actual + 1] & 0xFF);
            valorB = (int)inmediato;
            offset_actual+=2;
        } 
        else if (tipoB == 3) { 
            valorB = ((memoriaPrincipal[offset_actual] & 0xFF) << 16) | 
                     ((memoriaPrincipal[offset_actual + 1] & 0xFF) << 8) | 
                     (memoriaPrincipal[offset_actual + 2] & 0xFF);
            offset_actual+=3;
        }

        
        // dissasembler
        if (flag_d) {
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

        if (tipoA == 0) 
            registros[2] = 0; 
        else 
            registros[2] = (tipoA << 24) | (valorA & 0x00FFFFFF);

        if (tipoB == 0) 
            registros[3] = 0; 
        else 
            registros[3] = (tipoB << 24) | (valorB & 0x00FFFFFF);

        operaciones[opcode](registros, memoriaPrincipal, tablaSegmentos);

    }
}

void lecturaArchivo(char nombre[], short int tablaSegmentos[8][2], int registros[32], char memoriaPrincipal[ram]){
    FILE *arch = fopen(nombre, "rb");
    if (arch == NULL) {
        printf("Error al abrir el archivo.\n");
        exit(1);
    }
    char identificador[6]; 
    char version;
    char codigo;
    short int tamCod;
    

    fread(identificador, sizeof(char), 5, arch);
    identificador[5] = '\0'; 

    fread(&version, sizeof(char), 1, arch);

    fread(&tamCod, sizeof(short int), 1, arch);

    
    if (strcmp(identificador, "VMX26") != 0 || version != 1) {
        printf("Error: Archivo ejecutable invalido o version no soportada.\n");
        fclose(arch);
        exit(1); 
    }
    tablaSegmentos[0][0] = 0;
    tablaSegmentos[0][1] = tablaSegmentos[1][0] = tamCod;
    tablaSegmentos[1][1] = ram - tamCod;
    int i = 0;
    for(i=0; i < tamCod; i++){
        fread(&codigo, sizeof(char),1, arch);
        memoriaPrincipal[i]=codigo;
    }
    registros[0x1A] = 0;
    registros[0x1B] = 1 << 16;
    registros[0] = registros[0x1A];

    fclose(arch);

}

void iniciaSegmentos(short int matriz[8][2]){
    int i, j;
    for(i=2;i<8;i++){
        for(j=0; j<2; j++){
            matriz[i][j]= -1;
        }
    }
}

int main(int argc, char*argv[]) {
    short int tablaSegmentos[8][2];
    int registros[32] = {0};
    char memoriaPrincipal[ram] = {0};
    int flag_d=0;

    if(argc >= 2 && argc <= 3){
        if(argc == 3 && strcmp(argv[2],"-d") == 0){
            flag_d = 1;
        }
        iniciaSegmentos(tablaSegmentos);
        lecturaArchivo(argv[1], tablaSegmentos, registros, memoriaPrincipal); 
        ejecutarProceso(memoriaPrincipal, registros, tablaSegmentos, flag_d);

    }
    else
        printf("formato del pedido: ./{ejecutable} vmx {archivo.vmx} [-d]\n");

    return 0;
}

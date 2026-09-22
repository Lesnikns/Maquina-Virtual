#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "instrucciones.h"
#include "utilidades.h"
#include"disassembler.h"
#define ram 16384

int condicionProceso(int ip, short int tablaSegmentos[8][2]) {
    if (ip == -1) // si es stop
        return 0;

    int segmento = (ip >> 16) & 0xFFFF;
    int offset = ip & 0xFFFF;
    if (segmento < 0 || segmento > 7 || tablaSegmentos[segmento][0] == -1) { // si es segmento invalido
        return 0;
    }

    if (offset >= tablaSegmentos[segmento][1]) { // si termino de procesar el codesegment
        return 0;
    }
    return 1;
}
void ejecutarProceso(char memoriaPrincipal[ram], int registros[32], short int tablaSegmentos[8][2], int flag_d) {
    while (condicionProceso(registros[IP], tablaSegmentos)) {

        int segmento_ip = (registros[IP] >> 16) & 0xFFFF;
        int offset_ip = registros[IP] & 0xFFFF;
        int pc = tablaSegmentos[segmento_ip][0] + offset_ip;

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

        registros[OPC] = opcode; //pasamos a opc el codigo de operacion
        registros[IP] += (1 + tipoA + tipoB); //movemos ip
        
        int valorA = 0, valorB = 0;
        int offset_actual = pc + 1; 
        
        //primero se leera opA, luego opB
        if (tipoB == 1) {
            valorB = memoriaPrincipal[offset_actual] & 0xFF;
            offset_actual += 1;
        } 
        else
            if (tipoB == 2) {
                short inm = ((memoriaPrincipal[offset_actual]&0xFF)<<8)|(memoriaPrincipal[offset_actual+1]&0xFF);
                valorB=(int)inm; 
                offset_actual+=2;
            } 
            else
                if (tipoB == 3) {
                    valorB = ((memoriaPrincipal[offset_actual]&0xFF)<<16)|((memoriaPrincipal[offset_actual+1]&0xFF)<<8)|(memoriaPrincipal[offset_actual+2]&0xFF);
                    offset_actual+=3;
                }

        if (tipoA == 1) {
            valorA = memoriaPrincipal[offset_actual] & 0xFF;
            offset_actual += 1;
        } 
        else 
            if (tipoA == 2) {
                short inm = ((memoriaPrincipal[offset_actual]&0xFF)<<8)|(memoriaPrincipal[offset_actual+1]&0xFF);
                valorA=(int)inm;
                offset_actual+=2;
            } 
        else 
            if (tipoA == 3) {
                valorA = ((memoriaPrincipal[offset_actual]&0xFF)<<16)|((memoriaPrincipal[offset_actual+1]&0xFF)<<8)|(memoriaPrincipal[offset_actual+2]&0xFF);
                offset_actual+=3;
            }

        if (tipoA == 0) 
            registros[OP1] = 0;
        else 
            registros[OP1] = (tipoA << 24) | (valorA & 0x00FFFFFF);
        if (tipoB == 0) 
            registros[OP2] = 0;
        else 
            registros[OP2] = (tipoB << 24) | (valorB & 0x00FFFFFF);

        if (flag_d) {
            imprimirDisassembler(pc, opcode, tipoA, tipoB, valorA, valorB, memoriaPrincipal);
        }

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

    //printf("Identificador: %s\n", identificador);
    //printf("Version: %d\n", version);
    
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
        printf("formato del pedido: ./{ejecutable} {archivo-vmx} [-d]\n");

    return 0;
}

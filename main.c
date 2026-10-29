#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utilidades.h"
#include "disassembler.h"
#include "instrucciones.h"


int condicionProceso(int ip, short int tablaSegmentos[8][2], int registros[32]) {
    if (ip == -1) return 0;
    int segmento = (ip >> 16) & 0xFFFF;
    int offset = ip & 0xFFFF;
    int segmento_codigo = (registros[CS] >> 16) & 0xFFFF;

    // Solo sigue si el IP apunta específicamente al Code Segment
    if (segmento != segmento_codigo)
        return 0;
    if (offset >= (unsigned short)tablaSegmentos[segmento][1])
        return 0;
    return 1;
}
void ejecutarProceso(char memoriaPrincipal[], int registros[32], short int tablaSegmentos[8][2]) {
    while (condicionProceso(registros[IP], tablaSegmentos, registros)) {

        int segmento_ip = (registros[IP] >> 16) & 0xFFFF;
        int offset_ip = registros[IP] & 0xFFFF;
        int pc = (unsigned short)tablaSegmentos[segmento_ip][0] + offset_ip;

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

        operaciones[opcode](registros, memoriaPrincipal, tablaSegmentos);
    }
}
int calcularTamPS(char* nombreVmx, int numParametros, char** argsProceso) {
    if (nombreVmx == NULL || argsProceso == NULL) {
        return 0;
    }
    int tamPS = strlen(nombreVmx) + 1;
    for (int i = 0; i < numParametros; i++) {
        tamPS += strlen(argsProceso[i]) + 1;
    }
    int argc_total = numParametros + 1;
    tamPS += (argc_total * 4);
    return tamPS;
}
void configurarSegmentos(short int tablaSegmentos[8][2], int registros[32],
                         int tamPS, int tamKS, int tamCS, int tamDS, int tamES, int tamSS) {

    int tamanos[6] = {tamPS, tamKS, tamCS, tamDS, tamES, tamSS};
    int reg_indices[6] = {PS, KS, CS, DS, ES, SS};

    int base_actual = 0;
    int indice_tabla = 0;

    for (int i = 0; i < 6; i++) {
        if (tamanos[i] > 0) { // verificacion porque si o si existe CS y KS, puede que los demas no.
            tablaSegmentos[indice_tabla][0] = base_actual;
            tablaSegmentos[indice_tabla][1] = tamanos[i];
            registros[reg_indices[i]] = (indice_tabla << 16) | 0x0000; // ubico el registro la direccion

            base_actual += tamanos[i];
            indice_tabla++;
        } else {
            // Si el segmento no existe, el registro queda en -1[cite: 27]
            registros[reg_indices[i]] = -1;
        }
    }

    // relleno los que no tienen datos, queda compactado
    for (int i = indice_tabla; i < 8; i++) {
        tablaSegmentos[i][0] = -1;
        tablaSegmentos[i][1] = -1;
    }
}

void cargarVMI(char nombre[], short int tablaSegmentos[8][2], int registros[32], char memoriaPrincipal[], int tam_ram_bytes) {
    FILE *arch = fopen(nombre, "rb");
    if (arch == NULL) {
        printf("Error al abrir el archivo de imagen VMI.\n");
        exit(1);
    }

    char identificador[6];
    unsigned char version;
    leerExacto(identificador, 5, arch);
    identificador[5] = '\0';
    leerExacto(&version, 1, arch);

    if (strcmp(identificador, "VMI26") != 0 || version != 1) {
        printf("Error: Archivo de imagen VMI invalido o version no soportada.\n");
        fclose(arch);
        exit(1);
    }

    int tam_necesario_bytes = leerUShortEndian(arch) * 1024;
    if (tam_necesario_bytes != tam_ram_bytes) {
        printf("ERROR: La imagen requiere %d bytes, pero la maquina inicio con %d bytes.\n", tam_necesario_bytes, tam_ram_bytes);
        fclose(arch);
        exit(1);
    }

    for (int i = 0; i < 32; i++)
        registros[i] = leerIntEndian(arch);

    for (int i = 0; i < 8; i++) {
        tablaSegmentos[i][0] = leerShortEndian(arch); // Base
        tablaSegmentos[i][1] = leerShortEndian(arch); // Tamanio
    }

    leerExacto(memoriaPrincipal, tam_necesario_bytes, arch);
    fclose(arch);
}

void cargarVMX(char nombre[], short int tablaSegmentos[8][2], int registros[32], char memoriaPrincipal[], int tamRamBytes, char** argsProceso, int numParametros) {
    FILE *arch = fopen(nombre, "rb");
    if (arch == NULL) {
        printf("Error al abrir el archivo.\n");
        exit(1);
    }

    char identificador[6];
    unsigned char version;
    leerExacto(identificador, 5, arch);
    identificador[5] = '\0';
    leerExacto(&version, 1, arch);
    if (strcmp(identificador, "VMX26") != 0) {
        printf("Error: Archivo ejecutable invalido.\n");
        fclose(arch);
        exit(1);
    }

    int tamPS = 0, tamKS = 0, tamCS = 0, tamDS = 0, tamES = 0, tamSS = 0, offsetEP = 0;

    if (version == 1) {
        // MV1: solo codigo y datos; datos ocupa el resto de la memoria
        tamCS = leerUShortEndian(arch);
        tamDS = tamRamBytes - tamCS;
        if (tamDS > 0xFFFF) tamDS = 0xFFFF;
        if (tamDS < 0) tamDS = 0;
    } else if (version == 2) {
        tamCS    = leerUShortEndian(arch);
        tamDS    = leerUShortEndian(arch);
        tamES    = leerUShortEndian(arch);
        tamSS    = leerUShortEndian(arch);
        tamKS    = leerUShortEndian(arch);
        offsetEP = leerUShortEndian(arch);
        tamPS    = calcularTamPS(nombre, numParametros, argsProceso);
    } else {
        printf("Error: version de ejecutable no soportada (%d).\n", version);
        fclose(arch);
        exit(1);
    }

    int tams[6] = {tamPS, tamKS, tamCS, tamDS, tamES, tamSS};
    int total = 0;
    for (int i = 0; i < 6; i++) {
        if (tams[i] > 0xFFFF || (tams[i] > 0 && total > 0xFFFF)) { // tamanio o base no entran en 16 bits
            printf("ERROR: Memoria insuficiente. Segmento fuera del rango direccionable.\n");
            fclose(arch);
            exit(1);
        }
        total += tams[i];
    }
    if (total > tamRamBytes) {
        printf("ERROR: Memoria insuficiente. Requiere %d bytes, RAM: %d bytes.\n", total, tamRamBytes);
        fclose(arch);
        exit(1);
    }

    configurarSegmentos(tablaSegmentos, registros, tamPS, tamKS, tamCS, tamDS, tamES, tamSS);

    // Param Segment
    int offsetArgv = 0;
    if (tamPS > 0) {
        int psIdx = (registros[PS] >> 16) & 0xFFFF;
        int basePs = (unsigned short)tablaSegmentos[psIdx][0];
        int offsetActual = 0;
        int argcTotal = numParametros + 1;
        int offsetsStrings[argcTotal];

        strcpy(&memoriaPrincipal[basePs + offsetActual], nombre);
        offsetsStrings[0] = offsetActual;
        offsetActual += strlen(nombre) + 1;

        for (int i = 0; i < numParametros; i++) {
            strcpy(&memoriaPrincipal[basePs + offsetActual], argsProceso[i]);
            offsetsStrings[i + 1] = offsetActual;
            offsetActual += strlen(argsProceso[i]) + 1;
        }

        offsetArgv = offsetActual;
        for (int i = 0; i < argcTotal; i++) {
            int puntero_logico = (psIdx << 16) | offsetsStrings[i];
            memoriaPrincipal[basePs + offsetActual + 0] = (puntero_logico >> 24) & 0xFF;
            memoriaPrincipal[basePs + offsetActual + 1] = (puntero_logico >> 16) & 0xFF;
            memoriaPrincipal[basePs + offsetActual + 2] = (puntero_logico >> 8) & 0xFF;
            memoriaPrincipal[basePs + offsetActual + 3] = puntero_logico & 0xFF;
            offsetActual += 4;
        }
    }

    // Code Segment y Const Segment (en el archivo van en ese orden)
    if (tamCS > 0) {
        int baseCS = (unsigned short)tablaSegmentos[(registros[CS] >> 16) & 0xFFFF][0];
        leerExacto(&memoriaPrincipal[baseCS], tamCS, arch);
    }
    if (tamKS > 0) {
        int baseKS = (unsigned short)tablaSegmentos[(registros[KS] >> 16) & 0xFFFF][0];
        leerExacto(&memoriaPrincipal[baseKS], tamKS, arch);
    }

    registros[IP] = (registros[CS] & 0xFFFF0000) | (offsetEP & 0xFFFF);

    // Stack Segment
    if (tamSS > 0) {
        if (tamSS < 12) { // no entran RET, argc y argv
            printf("ERROR: STACK OVERFLOW\n");
            fclose(arch);
            exit(1);
        }
        int indiceSS = (registros[SS] >> 16) & 0xFFFF;
        int baseSS = (unsigned short)tablaSegmentos[indiceSS][0];
        int punteroSP = tamSS;

        int argcTotal = numParametros + 1;
        int argvLogico = (tamPS > 0) ? (int)((registros[PS] & 0xFFFF0000u) | (unsigned int)offsetArgv) : -1;
        int datosIniciales[3] = {argvLogico, (tamPS > 0) ? argcTotal : 0, -1};

        for (int i = 0; i < 3; i++) {
            punteroSP -= 4;
            int dir_fisica = baseSS + punteroSP;
            int valor = datosIniciales[i];
            memoriaPrincipal[dir_fisica + 0] = (valor >> 24) & 0xFF;
            memoriaPrincipal[dir_fisica + 1] = (valor >> 16) & 0xFF;
            memoriaPrincipal[dir_fisica + 2] = (valor >> 8) & 0xFF;
            memoriaPrincipal[dir_fisica + 3] = valor & 0xFF;
        }
        registros[SP] = (indiceSS << 16) | (punteroSP & 0xFFFF);
    } else {
        registros[SP] = -1;
    }
    fclose(arch);
}



int main(int argc, char* argv[]) {
    srand(time(NULL));

    char *archivoVMX = NULL;
    char *archivoVMI = NULL;
    int tamRamKib = 16; // Valor por defecto (16 KiB)
    int flag_d = 0;
    int paramIndex = -1;
    int numParametros = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-d") == 0) {
            flag_d = 1;
        } else if (strcmp(argv[i], "-p") == 0) {
            paramIndex = i + 1;
            numParametros = argc - paramIndex;
            break; // lo que sigue son parametros del programa, no de la VM
        } else if (strncmp(argv[i], "m=", 2) == 0) {
            tamRamKib = atoi(argv[i] + 2);
            if (tamRamKib <= 0 || tamRamKib > 64) {   // la tabla guarda direcciones de 16 bits
                printf("Error: el tamanio de la memoria debe estar entre 1 y 64 KiB.\n");
                return 1;
            }
        } else if (terminaEn(argv[i], ".vmx")) {
            archivoVMX = argv[i];
        } else if (terminaEn(argv[i], ".vmi")) {
            archivoVMI = argv[i];
        }
    }

    if (archivoVMX == NULL && archivoVMI == NULL) {
        printf("Error: Se requiere al menos un archivo .vmx o .vmi para ejecutar.\n");
        printf("Formato: vmx [filename.vmx] [filename.vmi] [m=M] [-d] [-p param1 ... paramN]\n");
        return 1;
    }

    // Solo .vmi: el tamanio de RAM lo define la imagen, no m=
    if (archivoVMX == NULL) {
        int kib = leerTamRamVMI(archivoVMI);
        if (kib <= 0) {
            printf("Error: no se pudo leer un archivo de imagen valido: %s\n", archivoVMI);
            return 1;
        }
        tamRamKib = kib;
    }

    int tamRamBytes = tamRamKib * 1024;
    char *memoriaPrincipal = (char*)calloc(tamRamBytes, sizeof(char));
    if (memoriaPrincipal == NULL) {
        printf("Error: No se pudo asignar la memoria principal.\n");
        return 1;
    }

    short int tablaSegmentos[8][2];
    int registros[32] = {0};

    if (archivoVMX != NULL) {
        char** argsProceso = (numParametros > 0) ? &argv[paramIndex] : NULL;
        cargarVMX(archivoVMX, tablaSegmentos, registros, memoriaPrincipal, tamRamBytes, argsProceso, numParametros);
    } else {
        cargarVMI(archivoVMI, tablaSegmentos, registros, memoriaPrincipal, tamRamBytes);
    }

    if (flag_d) {
        int offsetEP = registros[IP] & 0xFFFF; // Recuperamos el Entry Point
        volcadoDisassembler(memoriaPrincipal, tablaSegmentos, registros, offsetEP);
    }

    vmi_actual = archivoVMI;
    ram_actual = tamRamBytes;
    ejecutarProceso(memoriaPrincipal, registros, tablaSegmentos);

    free(memoriaPrincipal);
    return 0;
}
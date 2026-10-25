#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "utilidades.h"
#include "disassembler.h"

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
short int leerShortEndian(FILE *arch) {
    unsigned char bytes[2];
    fread(bytes, sizeof(unsigned char), 2, arch);
    return (short int)((bytes[0] << 8) | bytes[1]);
}
int leerIntEndian(FILE *arch) {
    unsigned char bytes[4];
    fread(bytes, sizeof(unsigned char), 4, arch);
    return (int)((bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3]);
}
void cargarVMI(char nombre[], short int tablaSegmentos[8][2], int registros[32], char memoriaPrincipal[], int tam_ram_bytes) {
    FILE *arch = fopen(nombre, "rb");
    if (arch == NULL) {
        printf("Error al abrir el archivo de imagen VMI.\n");
        exit(1);
    }

    char identificador[6];
    char version;

    // 1. Leer Identificador y Versión
    fread(identificador, sizeof(char), 5, arch);
    identificador[5] = '\0';
    fread(&version, sizeof(char), 1, arch);

    if (strcmp(identificador, "VMI26") != 0 || version != 1) {
        printf("Error: Archivo de imagen VMI invalido o version no soportada.\n");
        fclose(arch);
        exit(1);
    }

    // 2. Leer tamaño de la memoria (en KiB)
    short int tamMemoriaKiB = leerShortEndian(arch);
    int tam_necesario_bytes = tamMemoriaKiB * 1024;

    // Validar que la RAM asignada al ejecutar la máquina soporte la imagen guardada
    if (tam_necesario_bytes > tam_ram_bytes) {
        printf("ERROR: La imagen requiere %d bytes, pero la maquina inicio con %d bytes.\n", tam_necesario_bytes, tam_ram_bytes);
        fclose(arch);
        exit(1);
    }

    // 3. Leer los 32 registros (4 bytes cada uno, 128 bytes en total)
    for (int i = 0; i < 32; i++) {
        registros[i] = leerIntEndian(arch);
    }

    // 4. Leer la Tabla de Segmentos (8 filas x 2 columnas de 2 bytes = 32 bytes total)
    for (int i = 0; i < 8; i++) {
        tablaSegmentos[i][0] = leerShortEndian(arch); // Base
        tablaSegmentos[i][1] = leerShortEndian(arch); // Tamaño
    }

    // 5. Volcar la memoria principal completa[cite: 25]
    // Como es el estado exacto byte a byte, podemos hacer un único fread masivo
    fread(memoriaPrincipal, sizeof(char), tam_necesario_bytes, arch);

    fclose(arch);
}
void cargarVMX(char nombre[],short int tablaSegmentos[8][2], int registros[32], char memoriaPrincipal[], int tamRamBytes, char** argsProceso, int numParametros ) {
    FILE *arch = fopen(nombre, "rb");
    if (arch == NULL) {
        printf("Error al abrir el archivo.\n");
        exit(1);
    }
    char identificador[6];
    char version;

    fread(identificador, sizeof(char), 5, arch);
    identificador[5] = '\0';
    fread(&version, sizeof(char), 1, arch);
    if (strcmp(identificador, "VMX26") != 0) {
        printf("Error: Archivo ejecutable invalido.\n");
        fclose(arch);
        exit(1);
    }
    int tamCS = leerShortEndian(arch);
    int tamDS = leerShortEndian(arch);
    int tamES = leerShortEndian(arch);
    int tamSS = leerShortEndian(arch);
    int tamKS = leerShortEndian(arch);
    int offsetEP = leerShortEndian(arch);
    int tamPS = calcularTamPS(nombre, numParametros, argsProceso);

    int memoriaTotalRequerida = tamPS + tamKS + tamCS + tamDS + tamES + tamSS;
    if (memoriaTotalRequerida > tamRamBytes) {
        printf("ERROR: Memoria insuficiente. Requiere %d bytes, RAM: %d bytes.\n", memoriaTotalRequerida, tamRamBytes);
        fclose(arch);
        exit(1);
    }
    configurarSegmentos(tablaSegmentos, registros, tamPS, tamKS, tamCS, tamDS, tamES, tamSS);

    // cargar segmentos a la memoria
    int offsetArgv = 0;
    if (tamPS > 0) {
        int psIdx = registros[PS] >> 16;
        int basePs = tablaSegmentos[psIdx][0];
        int offsetActual = 0;

        // Arreglo temporal para guardar los offsets de cada string
        int argcTotal = numParametros + 1;
        int offsetsStrings[argcTotal];

        // Copiar nombre de archivo (.vmx)
        strcpy(&memoriaPrincipal[basePs + offsetActual], nombre);
        offsetsStrings[0] = offsetActual;
        offsetActual += strlen(nombre) + 1;

        // Copiar parametros extra
        for (int i = 0; i < numParametros; i++) {
            strcpy(&memoriaPrincipal[basePs + offsetActual], argsProceso[i]);
            offsetsStrings[i + 1] = offsetActual;
            offsetActual += strlen(argsProceso[i]) + 1;
        }

        // Construir el arreglo de punteros lógicos (argv)
        offsetArgv = offsetActual;
        for (int i = 0; i < argcTotal; i++) {
            int puntero_logico = (psIdx << 16) | offsetsStrings[i];

            // Guardar en Big Endian
            memoriaPrincipal[basePs + offsetActual + 0] = (puntero_logico >> 24) & 0xFF;
            memoriaPrincipal[basePs + offsetActual + 1] = (puntero_logico >> 16) & 0xFF;
            memoriaPrincipal[basePs + offsetActual + 2] = (puntero_logico >> 8) & 0xFF;
            memoriaPrincipal[basePs + offsetActual + 3] = puntero_logico & 0xFF;
            offsetActual += 4;
        }
    }

    if (tamCS > 0) {
        int indiceCS = registros[CS] >> 16;
        int baseCS = tablaSegmentos[indiceCS][0];
        fread(&memoriaPrincipal[baseCS], sizeof(char), tamCS, arch);
    }
    if (tamKS > 0) {
        int indiceKS = registros[KS] >> 16;
        int baseKS = tablaSegmentos[indiceKS][0];
        fread(&memoriaPrincipal[baseKS], sizeof(char), tamKS, arch);
    }

    registros[IP] = (registros[CS] & 0xFFFF0000) | (offsetEP & 0xFFFF);

    if (tamSS > 0) {
        int indiceSS = registros[SS] >> 16;
        int baseSS = tablaSegmentos[indiceSS][0];

        // SP empieza justo fuera del límite del stack (dirección lógica)
        int punteroSP = tamSS;

        int argvLogico = (tamPS > 0) ? ((registros[PS] & 0xFFFF0000) | offsetArgv) : -1;

        int argcTotal = numParametros + 1;
        // Función auxiliar local o bloque para hacer PUSH manual de 4 bytes a la memoria
        // Hacemos PUSH de argv, argc y RET (-1) respetando el orden
        int datosIniciales[3] = {argvLogico, (tamPS > 0) ? argcTotal : 0, -1};

        for (int i = 0; i < 3; i++) {
            punteroSP -= 4; // PUSH decrementa
            int dir_fisica = baseSS + punteroSP;
            int valor = datosIniciales[i];

            // Guardar en Big Endian
            memoriaPrincipal[dir_fisica + 0] = (valor >> 24) & 0xFF;
            memoriaPrincipal[dir_fisica + 1] = (valor >> 16) & 0xFF;
            memoriaPrincipal[dir_fisica + 2] = (valor >> 8) & 0xFF;
            memoriaPrincipal[dir_fisica + 3] = valor & 0xFF;
        }

        // Guardamos el SP en el registro
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
            break;
        } else if (strstr(argv[i], ".vmx") != NULL) {
            archivoVMX = argv[i];
        } else if (strstr(argv[i], ".vmi") != NULL) {
            archivoVMI = argv[i];
        }
    }

    if (archivoVMX == NULL && archivoVMI == NULL) {
        printf("Error: Se requiere al menos un archivo .vmx o .vmi para ejecutar.\n");
        printf("Formato: vmx [filename.vmx] [filename.vmi] [-m=M] [-d] [-p param1 ... paramN]\n");
        return 1;
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
    } else if (archivoVMI != NULL) {
        cargarVMI(archivoVMI, tablaSegmentos, registros, memoriaPrincipal, tamRamBytes);
    }


    if (flag_d) {
        int offsetEP = registros[IP] & 0xFFFF; // Recuperamos el Entry Point
        volcadoDisassembler(memoriaPrincipal, tablaSegmentos, registros, offsetEP);
    }

    extern char *vmi_actual;
    extern int ram_actual;
    vmi_actual = archivoVMI;
    ram_actual = tamRamBytes;
    ejecutarProceso(memoriaPrincipal, registros, tablaSegmentos);

    free(memoriaPrincipal);

    return 0;
}
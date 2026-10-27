#include "instrucciones.h"
#include "utilidades.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *vmi_actual = NULL;     // archivo .vmi recibido por parametro (NULL si no hay)
int ram_actual = 0;          // tamanio de la memoria principal en bytes
int bp_anidado = 0;          // 1 si la ultima instruccion ejecutada fue un breakpoint

int saltoValido(int destino, int reg[], short int seg[][2]) {
    int idx_cs = (reg[CS] >> 16) & 0xFFFF;
    int limiteCodeSeg = (unsigned short)seg[idx_cs][1];
    return destino >= 0 && destino < limiteCodeSeg;
}
void inst_sys (int reg[], char mem[], short int seg[][2]) {
    int op_a;
    int c;

    op_a = get_valor(reg[OP1],reg,mem,seg);

    switch (op_a) {

    case 0x0: // CLEAR SCREEN: no usa ni modifica registros
        fflush(stdout);
#ifdef _WIN32   // elige entre usar la sintaxis del clear de windows o linux en base a los requerimentos del sistema
        system("cls");
#else
        system("clear");
#endif
        break;

    case 0x1: // READ
    case 0x2: { // WRITE
        int cantByte = (reg[ECX] >> 16) & 0xFFFF; //cantidad de bytes a leer/escribir
        int cantValores = reg[ECX] & 0xFFFF; //cantidad de valores a leer/escribir
        int segmento    = (reg[EDX] >> 16) & 0xFFFF;
        int offset_base = reg[EDX] & 0xFFFF;

        // Validacion de Segmento (Solo si hay valores para operar)
        if (cantValores > 0 && cantByte > 0) {
            if (segmento < 0 || segmento > 7 || seg[segmento][0] == -1 ||
                (offset_base + (cantValores * cantByte)) > (unsigned short)seg[segmento][1]) {
                printf("Fallo de segmento\n");
                exit(1);
            }

            int edx = (unsigned short)seg[segmento][0] + offset_base; // direccion fisica base

            if(op_a == 1){ // Para lectura
                for(int i = 0; i < cantValores; i++){
                    int valor_a_guardar = 0;
                    int offset_actual = offset_base + (i * cantByte);
                    int dir_fisica_actual = edx + (i * cantByte);

                    // 1. Mostrar el prompt
                    printf("[%04X]: ", dir_fisica_actual);

                    // 2. Ingreso del dato
                    if((reg[EAX] & 0xF0) == 0x10){
                        char* cad_binaria = (char *)malloc(sizeof(char)*cantByte*8+1);
                        scanf("%s", cad_binaria);
                        for(int j=0; j<cantByte*8; j++){
                            valor_a_guardar = valor_a_guardar << 1;
                            if(cad_binaria[j] == '1')
                                valor_a_guardar += 1;
                        }
                        free(cad_binaria);
                    } else {
                        switch(reg[EAX] & 0xF){
                            case 0x1: scanf("%d", &valor_a_guardar); break;
                            case 0x2: { char ch; scanf(" %c", &ch); valor_a_guardar = (int)ch; } break;
                            case 0x4: scanf("%o", &valor_a_guardar); break;
                            case 0x8: scanf("%X", &valor_a_guardar); break;
                        }
                    }

                    // 3. Actualizar LAR, MAR y MBR
                    reg[LAR] = (segmento << 16) | (offset_actual & 0xFFFF);
                    reg[MAR] = (cantByte << 16) | (dir_fisica_actual & 0xFFFF);
                    reg[MBR] = valor_a_guardar;

                    // 4. Guardar en memoria
                    for(int j=0; j<cantByte; j++)
                        mem[dir_fisica_actual+j] = (valor_a_guardar >> (8*(cantByte-1-j))) & 0xFF;
                }
                // scanf deja el '\n' en el teclado: si no se descarta, el proximo
                // breakpoint lo toma como un Enter y entra en modo paso a paso
                while ((c = getchar()) != '\n' && c != EOF);
            }
            else { // Para escritura
                for(int i = 0; i < cantValores; i++){
                    int offset_actual = offset_base + (i * cantByte);
                    int dir_fisica_actual = edx + (i * cantByte);

                    int valor_a_escribir = mem[dir_fisica_actual] & 0xFF;
                    for(int j = 1; j < cantByte; j++){
                        valor_a_escribir = valor_a_escribir << 8;
                        valor_a_escribir += mem[dir_fisica_actual+j] & 0xFF;
                    }

                    // 1. Actualizar LAR, MAR y MBR
                    reg[LAR] = (segmento << 16) | (offset_actual & 0xFFFF);
                    reg[MAR] = (cantByte << 16) | (dir_fisica_actual & 0xFFFF);
                    reg[MBR] = valor_a_escribir;

                    // 2. Mostrar el prompt
                    printf("[%04X]: ", dir_fisica_actual);
                    int formato = reg[EAX];
                    int hexa = formato & 0x08;
                    int decimal = formato & 0x01;
                    int octal = formato & 0x04;
                    int binario = formato & 0x10;
                    int caracter = formato & 0x02;

                    if(binario == 0x10){
                        char *s = (char *)malloc(sizeof(valor_a_escribir)*8+1);
                        devuelveNotacionBinaria(valor_a_escribir, s);
                        printf("0b%s\n", s);
                        free(s);
                    }
                    if (hexa == 0x08) {
                        printf("0x%X ", valor_a_escribir);
                    }
                    if (octal == 0x04) {
                        printf("%o ", valor_a_escribir);
                    }
                    if (caracter == 0x02) {
                        printf("%c ", valor_a_escribir);
                    }
                    if (decimal == 0x01) {
                        printf("%d ", valor_a_escribir);
                    }
                    printf("\n");
                }
            }
        }
        break;
    }

    case 0x3: { // STRING READ: lee una linea y la guarda desde [EDX] terminada en '\0'
        int segmento = (reg[EDX] >> 16) & 0xFFFF;
        int offset   = reg[EDX] & 0xFFFF;
        if (segmento > 7 || seg[segmento][0] == -1 || offset >= (unsigned short)seg[segmento][1]) {
            printf("Fallo de segmento\n");
            exit(1);
        }
        int base       = (unsigned short)seg[segmento][0];
        int disponible = (unsigned short)seg[segmento][1] - offset; // bytes hasta el fin del segmento
        int maximo     = reg[ECX];                  // -1: sin limite
        int i = 0;

        while ((c = getchar()) != '\n' && c != EOF) {
            if (c == '\r')
                continue;                           // por si la entrada viene de Windows
            if (maximo >= 0 && i >= maximo)
                continue;                           // supera maximo, se descarta
            if (i + 1 >= disponible) {              // no entra caracter mas el '\0'
                printf("Fallo de segmento\n");
                exit(1);
            }
            mem[base + offset + i] = (char)c;
            i++;
        }
        mem[base + offset + i] = '\0';
        reg[LAR] = (segmento << 16) | ((offset + i) & 0xFFFF);
        reg[MAR] = (1 << 16) | ((base + offset + i) & 0xFFFF);
        reg[MBR] = 0;
        break;
    }

    case 0x4: { // STRING WRITE: imprime desde [EDX] hasta el '\0'
        int segmento = (reg[EDX] >> 16) & 0xFFFF;
        int offset   = reg[EDX] & 0xFFFF;
        if (segmento > 7 || seg[segmento][0] == -1 || offset >= (unsigned short)seg[segmento][1]) {
            printf("Fallo de segmento\n");
            exit(1);
        }
        int base   = (unsigned short)seg[segmento][0];
        int limite = (unsigned short)seg[segmento][1];
        int fin    = offset;

        while (fin < limite && mem[base + fin] != '\0')
            fin++;
        if (fin >= limite) { // string no termina dentro del segmento
            printf("Fallo de segmento\n");
            exit(1);
        }
        fwrite(&mem[base + offset], 1, fin - offset, stdout);
        fflush(stdout);
        reg[LAR] = (segmento << 16) | (fin & 0xFFFF);
        reg[MAR] = (1 << 16) | ((base + fin) & 0xFFFF);
        reg[MBR] = 0;
        break;
    }

    case 0xF: { // BREAKPOINT
        if (vmi_actual == NULL) // si no hay vmi se ignora
            break;
        bp_anidado = 1;

        while (1) {
            // 1. Imagen: header + 32 registros + tabla de segmentos + memoria (big endian)
            FILE *arch = fopen(vmi_actual, "wb");
            if (arch == NULL) {
                printf("ERROR: no se pudo crear el archivo de imagen %s\n", vmi_actual);
                exit(1);
            }
            unsigned char header[8] = { 'V', 'M', 'I', '2', '6', 1,
                                        ((ram_actual / 1024) >> 8) & 0xFF, (ram_actual / 1024) & 0xFF };
            fwrite(header, 1, 8, arch);

            for (int i = 0; i < 32; i++) {
                unsigned char b[4] = { (reg[i] >> 24) & 0xFF, (reg[i] >> 16) & 0xFF,
                                       (reg[i] >> 8) & 0xFF,  reg[i] & 0xFF };
                fwrite(b, 1, 4, arch);
            }
            for (int i = 0; i < 8; i++) {
                unsigned char b[4] = { (seg[i][0] >> 8) & 0xFF, seg[i][0] & 0xFF,    // base
                                       (seg[i][1] >> 8) & 0xFF, seg[i][1] & 0xFF };  // tamanio
                fwrite(b, 1, 4, arch);
            }
            fwrite(mem, 1, ram_actual, arch);
            fclose(arch);

            // 2. Espera la accion del usuario: g, q o Enter
            char linea[32];
            int accion = 0;                         // 'g' o '\n'
            while (accion == 0) {
                printf("[breakpoint] g: continuar | q: salir | Enter: paso a paso > ");
                fflush(stdout);

                if (fgets(linea, sizeof(linea), stdin) == NULL)  // sin entrada: continua
                    accion = 'g';
                else {
                    if (strchr(linea, '\n') == NULL)             // linea mas larga que el buffer
                        while ((c = getchar()) != '\n' && c != EOF);

                    if (linea[0] == '\n' || linea[0] == '\r')
                        accion = '\n';
                    else if (linea[0] == 'g' || linea[0] == 'G')
                        accion = 'g';
                    else if (linea[0] == 'q' || linea[0] == 'Q')
                        exit(0);                                 // vmi queda guardado
                    // cualquier otra tecla: se vuelve a preguntar
                }
            }
            if (accion == 'g')
                break;                              // ejecutarProceso sigue normalmente

            // 3. Enter: se ejecuta una instruccion y se vuelve a pausar.
            // Si programa ya termino, no hay mas a ejecutar.
            int seg_ip = (reg[IP] >> 16) & 0xFFFF;
            int off_ip = reg[IP] & 0xFFFF;
            if (reg[IP] == -1 || seg_ip > 7 || seg[seg_ip][0] == -1 || off_ip >= (unsigned short)seg[seg_ip][1])
                break;

            // lectura y decodificacion: lo mismo que hace ejecutarProceso
            int pc = (unsigned short)seg[seg_ip][0] + off_ip;
            unsigned char primer_byte = mem[pc];
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
            reg[OPC] = opcode;
            reg[IP] += (1 + tipoA + tipoB);

            int valorA = 0, valorB = 0;
            int pos = pc + 1;
            if (tipoB == 1) { valorB = mem[pos] & 0xFF; pos += 1; }
            else if (tipoB == 2) { valorB = (short)(((mem[pos] & 0xFF) << 8) | (mem[pos+1] & 0xFF)); pos += 2; }
            else if (tipoB == 3) { valorB = ((mem[pos] & 0xFF) << 16) | ((mem[pos+1] & 0xFF) << 8) | (mem[pos+2] & 0xFF); pos += 3; }
            if (tipoA == 1) { valorA = mem[pos] & 0xFF; }
            else if (tipoA == 2) { valorA = (short)(((mem[pos] & 0xFF) << 8) | (mem[pos+1] & 0xFF)); }
            else if (tipoA == 3) { valorA = ((mem[pos] & 0xFF) << 16) | ((mem[pos+1] & 0xFF) << 8) | (mem[pos+2] & 0xFF); }

            reg[OP1] = (tipoA == 0) ? 0 : ((tipoA << 24) | (valorA & 0x00FFFFFF));
            reg[OP2] = (tipoB == 0) ? 0 : ((tipoB << 24) | (valorB & 0x00FFFFFF));

            bp_anidado = 0;
            operaciones[opcode](reg, mem, seg);

            // si instruccion fue otro SYS F, breakpoint ya hizo su pausa
            if (bp_anidado)
                break;

            // si con esa instruccion termina programa, no se pausa de nuevo
            seg_ip = (reg[IP] >> 16) & 0xFFFF;
            off_ip = reg[IP] & 0xFFFF;
            if (reg[IP] == -1 || seg_ip > 7 || seg[seg_ip][0] == -1 || off_ip >= (unsigned short)seg[seg_ip][1])
                break;
        }
        bp_anidado = 1;
        break;
    }

    default:
        printf("ERROR: llamada al sistema invalida (SYS %X).\n", op_a);
        exit(1);
    }
}
void inst_jmp (int reg[], char mem[], short int seg[][2]) {
    int op = get_valor(reg[OP1],reg,mem,seg);

    if (saltoValido(op, reg, seg))
        reg[IP] = (reg[CS] & 0xFFFF0000) | (op & 0xFFFF); // Mantiene el segmento, actualiza offset
    else {
        printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
        exit(1);
    }
}

void inst_jp  (int reg[], char mem[], short int seg[][2]) {
    int n = (reg[CC] >> 31) & 0x1;
    int z = (reg[CC] >> 30) & 0x1;

    if(n == 0 && z == 0){
        int op = get_valor(reg[OP1],reg,mem,seg);
        if (saltoValido(op, reg, seg))
            reg[IP] = (reg[CS] & 0xFFFF0000) | (op & 0xFFFF);
        else{
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}

void inst_jn  (int reg[], char mem[], short int seg[][2]) {
    int n = (reg[CC] >> 31) & 0x1;

    if(n == 1){
        int op = get_valor(reg[OP1],reg,mem,seg);
        if (saltoValido(op, reg, seg))
            reg[IP] = (reg[CS] & 0xFFFF0000) | (op & 0xFFFF);
        else {
            printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
            exit(1);
        }
    }
}

void inst_jz  (int reg[], char mem[], short int seg[][2]){
    int z = (reg[CC] >> 30) & 0x1;

    if(z == 1){
        int op = get_valor(reg[OP1],reg,mem,seg); // CORREGIDO: antes decía reg[2]
        if (saltoValido(op, reg, seg))
            reg[IP] = (reg[CS] & 0xFFFF0000) | (op & 0xFFFF);
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
        if (saltoValido(opA, reg, seg))
            reg[IP] = (reg[CS] & 0xFFFF0000) | (opA & 0xFFFF);
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
        if (saltoValido(opA, reg, seg))
            reg[IP] = (reg[CS] & 0xFFFF0000) | (opA & 0xFFFF);
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
        if (saltoValido(opA, reg, seg))
            reg[IP] = (reg[CS] & 0xFFFF0000) | (opA & 0xFFFF);
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
        if (saltoValido(opA, reg, seg))
            reg[IP] = (reg[CS] & 0xFFFF0000) | (opA & 0xFFFF);
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
        if (saltoValido(opA, reg, seg))
            reg[IP] = (reg[CS] & 0xFFFF0000) | (opA & 0xFFFF);
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
void inst_push(int reg[], char mem[], short int seg[][2]) {
    if (reg[SS] == -1) {
        printf("ERROR: STACK OVERFLOW\n");
        exit(1);
    }
    int offsetSp = (reg[SP] & 0xffff) - 4;
    if (offsetSp < 0) {
        printf("ERROR: STACK OVERFLOW\n");
        exit(1);
    }
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int indiceSS = (reg[SS]>>16) & 0xffff;
    int dirFisica = (unsigned short)seg[indiceSS][0] + offsetSp;

    // guardo valor del opA respetando el bigendian
    mem[dirFisica] = (opA >> 24) & 0xff;
    mem[dirFisica + 1] = (opA >> 16) & 0xff;
    mem[dirFisica + 2] = (opA >> 8)  & 0xff;
    mem[dirFisica + 3] = opA & 0xff;

    reg[LAR] = offsetSp;    // Dirección lógica donde apilamos
    reg[MAR] = dirFisica;   // Dirección física real en memoria
    reg[MBR] = opA;

    reg[SP] = (reg[SS] & 0xFFFF0000) | (offsetSp & 0xFFFF); // actualizo sp con el nuevo offset por haber hecho push en la pila




}
void inst_pop(int reg[], char mem[], short int seg[][2]) {
    if (reg[SS] == -1) {
        printf("ERROR: STACK OVERFLOW\n");
        exit(1);
    }
    int offsetSP = reg[SP] & 0xFFFF;

    int idiceSS = (reg[SS] >> 16) & 0xFFFF;
    int limiteSS = (unsigned short)seg[idiceSS][1];

    if (offsetSP + 4 > limiteSS) { // si la pila esta vacia no puedo sacar nada
        printf("ERROR: STACK UNDERFLOW\n");
        exit(1);
    }
    int dirFisica = (unsigned short)seg[idiceSS][0] + offsetSP;

    int valExtraido = ((mem[dirFisica] & 0xFF) << 24) |
                   ((mem[dirFisica + 1] & 0xFF) << 16) |
                   ((mem[dirFisica + 2] & 0xFF) << 8) |
                   (mem[dirFisica + 3] & 0xFF);

    reg[LAR] = offsetSP;        // Dirección lógica de donde desapilamos
    reg[MAR] = dirFisica;       // Dirección física real en memoria
    reg[MBR] = valExtraido;

    set_valor(reg[OP1], valExtraido, reg, mem, seg);
    reg[SP] = (reg[SS] & 0xFFFF0000) | ((offsetSP + 4) & 0xFFFF);
}
void inst_call(int reg[], char mem[], short int seg[][2]) {
    if (reg[SS] == -1) {
        printf("ERROR: STACK OVERFLOW\n");
        exit(1);
    }
    int offsetSP = (reg[SP] & 0xFFFF) - 4;

    if (offsetSP < 0) {
        printf("ERROR: STACK OVERFLOW\n");
        exit(1);
    }
    int opA = get_valor(reg[OP1], reg, mem, seg);
    int idiceSS = (reg[SS] >> 16) & 0xFFFF;
    int dirFisica = (unsigned short)seg[idiceSS][0] + offsetSP;

    int valIP = reg[IP];
    mem[dirFisica]     = (valIP >> 24) & 0xFF;
    mem[dirFisica + 1] = (valIP >> 16) & 0xFF;
    mem[dirFisica + 2] = (valIP >> 8)  & 0xFF;
    mem[dirFisica + 3] = valIP & 0xFF;


    reg[LAR] = offsetSP;
    reg[MAR] = dirFisica;
    reg[MBR] = valIP;

    reg[SP] = (reg[SS] & 0xFFFF0000) | (offsetSP & 0xFFFF);

    if (saltoValido(opA, reg, seg)) {
        reg[IP] = (reg[CS] & 0xFFFF0000) | (opA & 0xFFFF);
    } else {
        printf("ERROR: Segmentation Fault. Salto fuera del Code Segment.\n");
        exit(1);
    }
}
void inst_ret(int reg[], char mem[], short int seg[][2]) {
    if (reg[SS] == -1) {
        printf("ERROR: STACK OVERFLOW\n");
        exit(1);
    }
    int offsetSP = reg[SP] & 0xFFFF;
    int idiceSS = (reg[SS] >> 16) & 0xFFFF;
    int limiteSS = (unsigned short)seg[idiceSS][1];


    if (offsetSP + 4 > limiteSS) {
        printf("STACK UNDERFLOW\n");
        exit(1);
    }

    int dirFisica = (unsigned short)seg[idiceSS][0] + offsetSP;

    // Extraigo la dirección de retorno q esta en la pila
    int valIP = ((mem[dirFisica] & 0xFF) << 24) |
                ((mem[dirFisica + 1] & 0xFF) << 16) |
                ((mem[dirFisica + 2] & 0xFF) << 8) |
                (mem[dirFisica + 3] & 0xFF);

    reg[LAR] = offsetSP;
    reg[MAR] = dirFisica;
    reg[MBR] = valIP;

    reg[IP] = valIP;  // Retauro el ip para volver

    reg[SP] = (reg[SS] & 0xFFFF0000) | ((offsetSP + 4) & 0xFFFF); // Actualizo el SP
}
void inst_stop(int reg[], char mem[], short int seg[][2]) {
    reg[IP] = -1; //carga en ip, -1 para indicar que termino la ejecucion del programa
}
void inst_mov (int reg[], char mem[], short int seg[][2]) {
    int op_b = get_valor(reg[OP2],reg,mem,seg);

    set_valor(reg[OP1], op_b, reg, mem, seg); //carga en A el valor de B
    actualizarCC(reg, op_b, 0, 0);
}
void inst_add(int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[OP1], reg, mem, seg);
    int op_b = get_valor(reg[OP2], reg, mem, seg);

    long long real_signed = (long long)op_a + (long long)op_b;
    unsigned long long real_unsigned = (unsigned long long)(unsigned int)op_a + (unsigned long long)(unsigned int)op_b;

    int res32_signed = (int)real_signed;
    unsigned int res32_unsigned = (unsigned int)real_unsigned;

    int overflow = (real_signed != res32_signed) ? 1 : 0; // compara el desbordamiento, si el resultado cambio a causa de que no entraba en los 32 bits.
    int carry = (real_unsigned != res32_unsigned) ? 1 : 0; // se fija que no haya quedado un bit suelto, acarreo

    set_valor(reg[OP1], res32_signed, reg, mem, seg);
    actualizarCC(reg, res32_signed, carry, overflow);
}
void inst_sub(int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg, mem, seg);
    int opB = get_valor(reg[OP2], reg, mem, seg);

    int res32_signed = opA - opB;
    long long real_signed = (long long)opA - (long long)opB;
    unsigned int comp2_opB = ~(unsigned int)opB + 1u;
    unsigned long long resU64 = (unsigned long long)(unsigned int)opA +
                                 (unsigned long long)comp2_opB;

    int overflow = (real_signed != res32_signed) ? 1 : 0; // compara el desbordamiento, si el resultado cambio a causa de que no entraba en los 32 bits.
    int carry = (resU64 >> 32) != 0 ? 1 : 0;// se fija que no haya quedado un bit suelto, acarreo


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

    int overflow = (real_signed != res32_signed) ? 1 : 0; // compara el desbordamiento, si el resultado cambio a causa de que no entraba en los 32 bits.
    int carry = (real_unsigned != res32_unsigned) ? 1 : 0; // se fija que no haya quedado un bit suelto, acarreo

    set_valor(reg[OP1], res32_signed, reg, mem, seg);
    actualizarCC(reg, res32_signed, carry, overflow);
}
void inst_div(int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg, mem, seg);
    int opB = get_valor(reg[OP2], reg, mem, seg);
    int cociente;
    int resto;
    int overflow = 0;

    if (opB == 0) { //  error division por cero
        printf("ERROR: Imposible dividir por cero.\n");
        exit(1);
    }

    if (opA == 0x80000000 && opB == -1) { // caso limite que genera desbordamiento
        cociente = 0x80000000;
        resto = 0;
        overflow = 1;
    } else {
        cociente = opA / opB;
        resto = opA % opB;
    }

    set_valor(reg[OP1], cociente, reg, mem, seg);
    reg[AC] = resto;
    actualizarCC(reg, cociente, 0, overflow);
}
void inst_cmp(int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg, mem, seg);
    int opB = get_valor(reg[OP2], reg, mem, seg);

    int res32_signed = opA - opB;
    long long real_signed = (long long)opA - (long long)opB;
    unsigned int comp2_opB = ~(unsigned int)opB + 1u;
    unsigned long long resU64 = (unsigned long long)(unsigned int)opA +
                                 (unsigned long long)comp2_opB;

    int overflow = (real_signed != res32_signed) ? 1 : 0;
    int carry = (resU64 >> 32) != 0 ? 1 : 0;

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

    int shift = op_b & 0x1F;
    unsigned int ua = (unsigned int)op_a;
    unsigned int ur = ua << shift;       // shift bien definido siempre
    int resultado = (int)ur;

    int carry = 0;
    if (shift > 0) {
        carry = (ua >> (32 - shift)) & 1;
    }

    long long real = (long long)op_a * (1LL << shift);
    int overflow = (real != resultado) ? 1 : 0;

    set_valor(reg[OP1], resultado, reg, mem, seg);
    actualizarCC(reg, resultado, carry, overflow);
}
void inst_shr (int reg[], char mem[], short int seg[][2]) {
    int opA = get_valor(reg[OP1], reg,mem, seg);
    int opB = get_valor(reg[OP2], reg,mem, seg);
    int carry = 0;
    int shift = opB & 0x1F; // evito que el valor de desplacamiento supere los 32 bits para que no rompa el programa
    unsigned int resultado = (unsigned int)opA >> shift;

    unsigned int ua = (unsigned int)opA;
    if (shift > 0)
        carry = (ua >> (shift - 1)) & 1; // aislo el bit que se cayo segun el desplamiento para calcular el carry

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
        int sector = (valor_crudo >> 5) & 0x03; // Extraemos los 2 bits de sector

        // Aplicamos la máscara correspondiente para sobrescribir solo la parte solicitada
        if (sector == 1) { // LOW
            registros[reg] = (registros[reg] & 0xFFFFFF00) | (valor_a_guardar & 0xFF);
        }
        else if (sector == 2) { // HIGH
            registros[reg] = (registros[reg] & 0xFFFF00FF) | ((valor_a_guardar & 0xFF) << 8);
        }
        else if (sector == 3) { // 2 BYTES
            registros[reg] = (registros[reg] & 0xFFFF0000) | (valor_a_guardar & 0xFFFF);
        }
        else { // 4 BYTES
            registros[reg] = valor_a_guardar;
        }
        return;
    }

    if (tipo == 3) {
        short offset = (short)(valor_crudo >> 8);
        int reg = valor_crudo & 0x1F;
        int modificador = (valor_crudo >> 5) & 0x07; // Extraemos los 3 bits de tamaño

        // Determinamos la cantidad de bytes a escribir
        int cant_bytes = 4; // Por defecto es long (100)
        if (modificador == 1) cant_bytes = 1;      // 001: byte (b)
        else if (modificador == 2) cant_bytes = 2; // 010: word (w)

        int puntero_logico = registros[reg];
        int segmento = (puntero_logico >> 16) & 0xFFFF;
        int offset_base = puntero_logico & 0xFFFF;

        if (segmento < 0 || segmento > 7 || tablaSegmentos[segmento][0] == -1) {
            printf("ERROR: Segmentation Fault. Segmento invalido o no inicializado. \n");
            exit(1);
        }

        int offset_final = offset_base + offset;

        // Validación dinámica de límites según la cantidad de bytes a escribir
        if (offset_final < 0 || (offset_final + cant_bytes - 1) >= (unsigned short)tablaSegmentos[segmento][1]) {
            printf("ERROR: Segmentation Fault. Acceso fuera de los limites del segmento.\n");
            exit(1);
        }

        int dir_fisica = (unsigned short)tablaSegmentos[segmento][0] + offset_final;

        // Carga de registro LAR: Dirección Lógica
        registros[LAR] = (segmento << 16) | (offset_final & 0xFFFF);

        // Carga de registro MAR: Cantidad de bytes a escribir + Dirección Física
        registros[MAR] = (cant_bytes << 16) | (dir_fisica & 0xFFFF);

        // Carga de registro MBR: Valor a escribir
        registros[MBR] = valor_a_guardar;

        // Escritura en memoria respetando el tamaño y truncando los bytes más significativos sobrantes
        if (cant_bytes == 1) {
            memoriaPrincipal[dir_fisica] = valor_a_guardar & 0xFF;
        }
        else if (cant_bytes == 2) {
            memoriaPrincipal[dir_fisica]     = (valor_a_guardar >> 8) & 0xFF;
            memoriaPrincipal[dir_fisica + 1] = valor_a_guardar & 0xFF;
        }
        else {
            memoriaPrincipal[dir_fisica]     = (valor_a_guardar >> 24) & 0xFF;
            memoriaPrincipal[dir_fisica + 1] = (valor_a_guardar >> 16) & 0xFF;
            memoriaPrincipal[dir_fisica + 2] = (valor_a_guardar >> 8) & 0xFF;
            memoriaPrincipal[dir_fisica + 3] = valor_a_guardar & 0xFF;
        }
        return;
    }
}
int get_valor(int operando_empaquetado, int registros[], char memoriaPrincipal[], short int tablaSegmentos[8][2]) {
    int tipo = (unsigned int)operando_empaquetado >> 24;
    int valor_crudo = operando_empaquetado & 0x00FFFFFF;

    if (tipo == 0) return 0; // Ningun operando

    if (tipo == 1) { // Registro
        int reg = valor_crudo & 0x1F;
        int sector = (valor_crudo >> 5) & 0x03; // Extraemos los 2 bits de sector
        int valor_completo = registros[reg];

        // Retornamos según el sector solicitado propagando signo si es necesario
        if (sector == 1) { // LOW
            return (int)((char)(valor_completo & 0xFF));
        }
        else if (sector == 2) { // HIGH
            return (int)((char)((valor_completo >> 8) & 0xFF));
        }
        else if (sector == 3) { // 2 BYTE
            return (int)((short)(valor_completo & 0xFFFF));
        }

        return valor_completo; // 4 BYTES
    }

    if (tipo == 2) { // Inmediato
        short inmediato = (short)(valor_crudo & 0xFFFF);
        return (int)inmediato;
    }

    if (tipo == 3) { // Memoria
        short offset = (short)(valor_crudo >> 8);
        int reg = valor_crudo & 0x1F;
        int modificador = (valor_crudo >> 5) & 0x07; // Extraemos los 3 bits de tamaño

        int cant_bytes = 4; // Por defecto es long (100)
        if (modificador == 1) cant_bytes = 1;      // 001: byte (b)
        else if (modificador == 2) cant_bytes = 2; // 010: word (w)

        int puntero_logico = registros[reg];
        int segmento = (puntero_logico >> 16) & 0xFFFF;
        int offset_base = puntero_logico & 0xFFFF;

        if (segmento < 0 || segmento > 7 || tablaSegmentos[segmento][0] == -1) {
            printf("ERROR: Segmentation Fault. Segmento invalido o no inicializado.\n");
            exit(1);
        }

        int offset_final = offset_base + offset;

        // Verificamos que todos los bytes que vamos a leer entren en el segmento
        if (offset_final < 0 || (offset_final + cant_bytes - 1) >= (unsigned short)tablaSegmentos[segmento][1]) {
            printf("ERROR: Segmentation Fault. Acceso fuera de los limites del segmento.\n");
            exit(1);
        }

        int dir_fisica = (unsigned short)tablaSegmentos[segmento][0] + offset_final;

        // Carga de registro LAR: Dirección Lógica (Segmento + Offset Final)
        registros[LAR] = (segmento << 16) | (offset_final & 0xFFFF);

        // Carga de registro MAR: Cantidad de bytes a leer + Dirección Física
        registros[MAR] = (cant_bytes << 16) | (dir_fisica & 0xFFFF);

        int dato = 0;

        // Lectura de memoria respetando el tamaño y propagando signo a 32 bits
        if (cant_bytes == 1) {
            char b = memoriaPrincipal[dir_fisica];
            dato = (int)b;
        }
        else if (cant_bytes == 2) {
            short w = (short)(((memoriaPrincipal[dir_fisica] & 0xFF) << 8) |
                              (memoriaPrincipal[dir_fisica + 1] & 0xFF));
            dato = (int)w;
        }
        else {
            dato = ((memoriaPrincipal[dir_fisica] & 0xFF) << 24) |
                   ((memoriaPrincipal[dir_fisica + 1] & 0xFF) << 16) |
                   ((memoriaPrincipal[dir_fisica + 2] & 0xFF) << 8) |
                   (memoriaPrincipal[dir_fisica + 3] & 0xFF);
        }

        // Carga de registro MBR: Valor leído de memoria
        registros[MBR] = dato;

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
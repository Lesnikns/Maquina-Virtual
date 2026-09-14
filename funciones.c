void inst_sys (int reg[], char mem[], short int seg[][2]){
    if(reg[2]==0x1){ //para lectura
        //guardar en memoria en pos que apunta edx, segun 2 bytes mas signif de ecx te da cant. de bits a leer,
        //y los 2 menos, la cantidad de valores a leer + segun eax el tipo de dato a leer

        int cantByte = reg[12] >> 16; //cantidad de bytes a leer, si cada celda de memoria es de 1 byte, entonces leer al menos una celda?
        int cantValores = reg[12] & 0xFFFF; //cantidad de valores a leer
        for(int i=0;i<cantValores;i++){
            int valor_a_guardar;
            scanf("%d", &valor_a_guardar);
            for(int j=0;j<cantByte;j++){
                mem[reg[13]+i*cantByte+j] = (valor_a_guardar >> (8*(cantByte-1-j))) & 0xFF; //comienza en cantbyte-1 para leer el primer byte mas significativo, y luego va bajando
            }
        }
    }
    else
        if(reg[2]==0x2){ //para escritura
            int cantByte = reg[12] >> 16; //cantidad de bytes a escribir, si cada celda de memoria es de 1 byte, entonces escribir al menos una celda?
            int cantValores = reg[12] & 0xFFFF; //cantidad de valores a escribir
            for(int i=0;i<cantValores;i++){
                int valor_a_escribir = mem[reg[13]+i];
                for(int j=1; j < cantByte; j++){
                    valor_a_escribir = valor_a_escribir << 8;
                    valor_a_escribir += mem[reg[13]+i+j];
                }
                if(reg[10] & 0xF0 == 0x10){ //imprime en binario
                    char *s = (char *)malloc(sizeof(valor_a_escribir)*8);
                    devuelveNotacionBinaria(valor_a_escribir,s);
                    printf("%s", s);
                    free(s);
                }
                else
                    switch(reg[10] & 0xF){
                        case 0x0: //imprime en decimal
                            printf("%d", &valor_a_escribir);
                            break;
                        case 0x1: //caracter
                            printf("%c", &valor_a_escribir);
                            break;
                        case 0x4: //octal
                            printf("%o", &valor_a_escribir);
                            break;
                        case 0x8: //hexadecimal
                            printf("%X", &valor_a_escribir);
                            break;
                        default:
                            printf("Error: llamada al sistema invalida.\n");
                            exit(1);
                    }
            }
        }
        else{
            printf("Error: llamada al sistema invalida.\n");
            exit(1);
        }
}
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
void inst_sub (int reg[], char mem[], short int seg[][2]);
void inst_mul (int reg[], char mem[], short int seg[][2]);
void inst_div (int reg[], char mem[], short int seg[][2]);
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
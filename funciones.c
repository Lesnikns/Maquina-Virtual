void inst_sys (int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[2],reg,mem,seg);
    int cantByte = reg[12] >> 16; //cantidad de bytes a leer/escribir
    int cantValores = reg[12] & 0xFFFF; //cantidad de valores a leer/escribir
    int segmento    = (reg[13] >> 16) & 0xFFFF;
    int offset_base = reg[13] & 0xFFFF;
    int edx = seg[segmento][0] + offset_base; // direccion fisica real para leer/escribir datos

    if(op_a == 1){ //para lectura
        for(int i=0;i<cantValores;i++){
            int valor_a_guardar = 0;
            if((reg[10] & 0xF0) == 0x10){ //lee entrada en formato binario
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
                switch(reg[10] & 0xF){
                    case 0x0: //imprime en decimal
                        scanf("%d", &valor_a_guardar);
                        break;
                    case 0x1: //caracter
                        scanf("%c", &valor_a_guardar);
                        break;
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

                if(((reg[10] & 0xF0) == 0x10)){ //imprime en binario
                    char *s = (char *)malloc(sizeof(valor_a_escribir)*8+1);
                    devuelveNotacionBinaria(valor_a_escribir,s);
                    printf("%s", s);
                    free(s);
                }
                else
                    switch(reg[10] & 0xF){
                        case 0x0: //imprime en decimal
                            printf("%d", valor_a_escribir);
                            break;
                        case 0x1: //caracter
                            printf("%c", valor_a_escribir);
                            break;
                        case 0x4: //octal
                            printf("%o", valor_a_escribir);
                            break;
                        case 0x8: //hexadecimal
                            printf("%X", valor_a_escribir);
                            break;
                    }
            }
        }
}
void inst_jmp (int reg[], char mem[], short int seg[][2]) {
    int op = get_valor(reg[2],reg,mem,seg);
    set_valor(reg[0], op, reg, mem, seg); //carga en ip el valor de op, bien hecho?
}
void inst_jp  (int reg[], char mem[], short int seg[][2]) {
    int n = reg[17] & 0x0FFFFFFF >> 31 & 0x1; //bit de signo negativo
    
    if(n == 0){ //si el bit de signo negativo es 0, entonces es positivo
        int op = get_valor(reg[2],reg,mem,seg);
        set_valor(reg[0], op, reg, mem, seg); //carga en ip el valor de op
    }
}
void inst_jn  (int reg[], char mem[], short int seg[][2]) {
    int n = reg[17] & 0x0FFFFFFF >> 31 & 0x1; //bit de signo negativo

    if(n == 1){ //si el bit de signo negativo es 1, entonces es negativo
        int op = get_valor(reg[2],reg,mem,seg);
        set_valor(reg[0], op, reg, mem, seg); //carga en ip el valor de op
    }
}
void inst_jz  (int reg[], char mem[], short int seg[][2]){
    int z = reg[17] & 0x0FFFFFFF >> 30 & 0x1; //bit de cero

    if(z == 0){ //si el bit de cero es 0, entonces es diferente de cero
        int op = get_valor(reg[2],reg,mem,seg);
        set_valor(reg[0], op, reg, mem, seg); //carga en ip el valor de op
    }
}
void inst_jc  (int reg[], char mem[], short int seg[][2]);
void inst_jv  (int reg[], char mem[], short int seg[][2]);
void inst_jnp (int reg[], char mem[], short int seg[][2]);
void inst_jnn (int reg[], char mem[], short int seg[][2]);
void inst_jnz (int reg[], char mem[], short int seg[][2]);
void inst_not (int reg[], char mem[], short int seg[][2]);
void inst_invalida(int reg[], char mem[], short int seg[][2]) {
    printf("INSTRUCCION INVALIDA");
}
void inst_stop(int reg[], char mem[], short int seg[][2]) {
    set_valor(reg[0], -1, reg, mem, seg); //carga en ip, -1 para indicar que termino la ejecucion del programa
}
void inst_mov (int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[2],reg,mem,seg);
    int op_b = get_valor(reg[3],reg,mem,seg);

    set_valor(op_a, op_b, reg, mem, seg); //carga en A el valor de B
    actualizarCC(reg, op_b, 0, 0); //bien?
}
void inst_add (int reg[], char mem[], short int seg[][2]){
    int op_a = get_valor(reg[2],reg,mem,seg);
    int op_b = get_valor(reg[3],reg,mem,seg);

    int resultado = op_a + op_b;
    set_valor(reg[2], resultado, reg, mem, seg);

    int carry = (resultado < op_a) ? 1 : 0;
    int overflow = ((op_a > 0 && op_b > 0 && resultado < 0) || (op_a < 0 && op_b < 0 && resultado > 0)) ? 1 : 0;
    actualizarCC(reg, resultado, carry, overflow);
}
void inst_sub (int reg[], char mem[], short int seg[][2]);
void inst_mul (int reg[], char mem[], short int seg[][2]);
void inst_div (int reg[], char mem[], short int seg[][2]);
void inst_cmp (int reg[], char mem[], short int seg[][2]) {
    
    int op_a = get_valor(reg[2],reg,mem,seg);
    int op_b = get_valor(reg[3],reg,mem,seg);

    int resultado = op_a - op_b;

    int carry = (op_a < op_b) ? 1 : 0;
    int overflow = ((op_a > 0 && op_b < 0 && resultado < 0) || (op_a < 0 && op_b > 0 && resultado > 0)) ? 1 : 0;
    actualizarCC(reg, resultado, carry, overflow);
}
void inst_and (int reg[], char mem[], short int seg[][2]) {
    
    int op_a = get_valor(reg[2],reg,mem,seg);
    int op_b = get_valor(reg[3],reg,mem,seg);

    int resultado = op_a & op_b;
    set_valor(reg[2], resultado, reg, mem, seg);

    actualizarCC(reg, resultado, 0, 0);
}
void inst_or  (int reg[], char mem[], short int seg[][2]);
void inst_xor (int reg[], char mem[], short int seg[][2]) {
    
    int op_a = get_valor(reg[2],reg,mem,seg);
    int op_b = get_valor(reg[3],reg,mem,seg);

    int resultado = op_a ^ op_b;
    set_valor(reg[2], resultado, reg, mem, seg);

    actualizarCC(reg, resultado, 0, 0);
}
void inst_swap(int reg[], char mem[], short int seg[][2]);
void inst_shl (int reg[], char mem[], short int seg[][2]) {
    
    int op_a = get_valor(reg[2],reg,mem,seg);
    int op_b = get_valor(reg[3],reg,mem,seg);

    int resultado = op_a << op_b;
    set_valor(reg[2], resultado, reg, mem, seg);

    int carry = (op_a & (1 << (32 - op_b))) ? 1 : 0; 
    int overflow = ((op_a > 0 && resultado < 0) || (op_a < 0 && resultado > 0)) ? 1 : 0;
    actualizarCC(reg, resultado, carry, overflow);
}
void inst_shr (int reg[], char mem[], short int seg[][2]);
void inst_sar (int reg[], char mem[], short int seg[][2]);
void inst_ldl (int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[2],reg,mem,seg);
    int op_b = get_valor(reg[3],reg,mem,seg);

    //carga los 2 bytes menos significativos del A, con los 2 bytes menos significativos de B
    int resultado = (op_a & 0xFFFF0000) | (op_b & 0x0000FFFF); 
    set_valor(reg[2], resultado, reg, mem, seg);
}
void inst_ldh (int reg[], char mem[], short int seg[][2]);
void inst_rnd (int reg[], char mem[], short int seg[][2]) {
    int op_a = get_valor(reg[2],reg,mem,seg);
    int op_b = get_valor(reg[3],reg,mem,seg);

    //genera numero aleatorio entre 0 y op_b, y lo guarda en A
    int resultado = rand() % (op_b + 1);
    set_valor(op_a, resultado, reg, mem, seg);
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
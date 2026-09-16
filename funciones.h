enum Registros {
    IP = 0, OPC = 1, OP1 = 2, OP2 = 3,
    LAR = 4, MAR = 5, MBR = 6,
    // 7, 8, 9 están reservados
    EAX = 10, EBX = 11, ECX = 12, EDX = 13, EEX = 14, EFX = 15,
    AC = 16, CC = 17,
    // 18 al 25 están reservados
    CS = 26, DS = 27
};
int saltoValido(int destino, short int seg[][2]);
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
void inst_invalida(int reg[], char mem[], short int seg[][2]); // para las funciones que todavia no desbloqueamos
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
void set_valor(int operando_empaquetado, int valor_a_guardar, int registros[], char memoriaPrincipal[], short int tablaSegmentos[8][2]);
int get_valor(int operando_empaquetado, int registros[], char memoriaPrincipal[], short int tablaSegmentos[8][2]);
void actualizarCC(int registros[], int resultado, int carry, int overflow);

void devuelveNotacionBinaria(int nro,char *s); //para impresion numeros en binario
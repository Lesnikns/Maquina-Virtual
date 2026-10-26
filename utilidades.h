#include <stdio.h>

void leerExacto(void *dst, size_t n, FILE *arch);
unsigned short leerUShortEndian(FILE *arch);
short int leerShortEndian(FILE *arch);
int leerIntEndian(FILE *arch);
int terminaEn(const char *s, const char *ext);
int leerTamRamVMI(const char *ruta);
typedef void (*InstruccionFunc)(int registros[], char memoriaPrincipal[], short int tablaSegmentos[8][2]);
extern InstruccionFunc operaciones[32];
extern const char* mnemonicos[32];
extern const char* nom_regs[32];
enum Registros {
    IP = 0, OPC = 1, OP1 = 2, OP2 = 3,
    LAR = 4, MAR = 5, MBR = 6, SP = 7, BP = 8,
    // 9 está reservado
    EAX = 10, EBX = 11, ECX = 12, EDX = 13, EEX = 14, EFX = 15,
    AC = 16, CC = 17,
    // 18 al 25 están reservados
    CS = 26, DS = 27, ES = 28, SS = 29, KS = 30, PS = 31
};

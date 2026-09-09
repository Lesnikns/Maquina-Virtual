#include <stdio.h>
#include <stdlib.h>
#define ram 16384

typedef struct{
    char car;
} registrobyte;

typedef struct{
    short int num;
}registro2byte;


void lecturaArchivo(char nombre[], short int tablaSegmentos[8][2], int registros[32], char memoriaPrincipal[ram]){
    FILE *arch = fopen(nombre, "rb");
    registrobyte r1;
    registro2byte tamCod;
    fread(&r1, sizeof(registrobyte), 6, arch);
    fread(&tamCod, sizeof(registro2byte), 1, arch);
    tablaSegmentos[0][0] = 0;
    tablaSegmentos[0][1] = tablaSegmentos[1][0] = tamCod.num;
    tablaSegmentos[1][1] = ram - tamCod.num;
    int i = 0;
    for(i=0; i < tamCod.num; i++){
        fread(&r1, sizeof(registrobyte),1, arch);
        memoriaPrincipal[i]=r1.car;
    }
    registros[0x1A] = 0;
    registros[0x1B] = tamCod.num;
    registros[0x1B] = registros[0x1B] << 8;


}

void iniciaSegmentos(short int matriz[8][2]){
    int i, j;
    for(i=2;i<8;i++){
        for(j=0; j<2; j++){
            matriz[i][j]= 0xf;
        }
    }
}

int main(int argc, char*argv[]){

    short int tablaSegmentos[8][2]= {0xf};
    char nombre[10];
    iniciaSegmentos(tablaSegmentos);
    int registros[32];
    char memoriaPrincipal[ram];
    lecturaArchivo(nombre, tablaSegmentos, registros, memoriaPrincipal);
}

# Maquina-Virtual
Trabajo práctico para fundamentos de arquitectura de computadoras.

## Arquitectura y características
- Tabla de segmentos: Representa la unidad de gestión de memoria. Está representada por una matriz donde las 8 filas representan los segmentos disponibles. La columna [0] almacena la dirección base física del segmento en la RAM, y la columna [1] almacena el límite del segmento. Para establecer que las instrucciones no realicen saltos fuera del segmento correspondiente y para controlar el fin de proceso.
- Memoria Principal: Simula la memoria RAM del sistema como un arreglo lineal continuo de bytes. Todo acceso de lectura o escritura de operandos (a través de get_valor y set_valor) y toda lectura de código (a través del IP) impacta físicamente en este arreglo tras traducir las direcciones lógicas de la tabla de segmentos.
- Registros: Es nuestra trabla de registros del procesador, allí almacenamos los registros que usará tanto el programador assembler como la máquina virtual para operar. 

## Decisiones de diseño implentadas
- Librería instrucciones.h / .c: Decidimos separar la lógica de todas las instrucciones que realiza la máquina en una librería independiente para mejorar la claridad del código, apuntando a una buena escalabilidad y mantenimiento.
- Librería utilidades.h / .c: Creamos esta librería para aislar los vectores constantes (nombres de registros y mnemónicos) y, fundamentalmente, el arreglo de punteros a funciones (operaciones[32]). Esta decisión nos permitió eliminar grandes bloques de switch y dejar el ciclo de ejecución en main.c lo más limpio y rápido posible.
- Librería disassembler.c: Apartamos la lógica de formato e impresión del disassembler en un módulo aparte para no saturar el motor principal de procesamiento ni el ciclo while, manteniendo la responsabilidad de "ejecución" separada de la "visualización".
- Funciones set_valor y get_valor: Optamos por centralizar el acceso a datos. get_valor se encarga de decodificar el operando empaquetado, devolviendo el número matemático correcto sin importar si provenía de un registro, de la memoria o si era un inmediato. Por el contrario, set_valor abstrae la complejidad de guardar el resultado de la instrucción en su destino final, incluyendo las barreras de seguridad de memoria (Segmentation Fault). Estas funciones evitan la duplicación masiva de código en cada instrucción individual.

**Para compilar usando GCC desde la terminal:**
```bash
gcc main.c instrucciones.c disassembler.c utilidades.c -o vm

./vm [archivo_binario.vmx]
```
**Para disassembler: **
```bash
./vm [archivo_binario.vmx] -d 
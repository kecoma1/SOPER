/**
 * @file Utils.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiante.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Modulo que contiene funciones que proveen 
 * utilidades de carácter general.
 * 
 * Modulo: Utils
 * 
 * @version 1
 * @date 2020-04-28
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include "Global.h"
#include "Utils.h"

/* Prototipos funciones privadas */
int potencia2(int n);

/* Funciones públicas */
/**
 * @brief Calcula el logaritmo entero en base 2
 * 
 * @param n Numero a calcular
 * @return int Resultado de la operación
 */
int compute_log(int n) {
    int log = 0;
    int i = 0;
    
    if (n == 0) return -1;
    if (n == 1) return 1;

    for (i = 0; log <= n; i++) {
        log = potencia2(i);
    }

    return i-2;
}

/**
 * @brief Imprime un vector por pantalla en modo texto
 * 
 * @param vector Vector a imprimir
 * @param len Longitud del vector
 * @return Status Ejecución de la función
 */
Status print_vector(int *vector, int len) {
    if (vector == NULL || len <= 0) return ERR;

    for(int i = 0; i < len-1; i++) printf("%d - ", vector[i]);

    printf("%d", vector[len-1]);

    return OK;
}

/**
 * @brief Pone a dormir un proceso durante un cierto número de nanosegundos
 * 
 * @param nanoseconds Nanosegundos que debe dormir el proceso
 * @return Status Ejecución de la función
 */
Status fast_sleep(int nanoseconds) {
    if(nanoseconds <= 0) return ERR;

    struct timespec t;

    t.tv_nsec = (long)nanoseconds;
    t.tv_sec = (time_t)0;

    if (nanosleep(&t, NULL)) {
        perror("nanosleep: ");
        return ERR;
    }

    return OK;
}

/******************* Funciónes privadas *******************/
/**
 * @brief Calcula 2^n
 * 
 * @param n Potencia
 * @return int Resultado
 */
int potencia2(int n) {
    int resultado = 2;

    if (n == 0) return 1;
    else if (n == 1) return 2;
    else if (n < 0) return -1;

    for(int i = 1; i < n; i++) resultado *= 2;

    return resultado;
}

void main() {
    printf("%d\n", compute_log(5));
    printf("%d\n", compute_log(30));
    fast_sleep(100000000);
    printf("%d\n", compute_log(128));
    printf("%d\n", compute_log(64));
}
/**
 * @file ejercicio_kill.c
 * @author 
 *      Kevin de la Coba Malam (kevin.coba@estudiante.uam.es)
 *      Jose Manuel Freire (jose.freire@estudiante.uam.es)
 * @brief Programa para mandar señales a procesos
 * @version 1.0
 * @date 2020-03-14
 *
 * - Grupo: 2291
 * - Pareja: 5
 *  
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

/* Asumimos que la SIGNAL no va a tener más de 10 dígitos */
#define MAXLENSIGNAL 10

int main(int argc, char **argv) {

    char SIGNAL[MAXLENSIGNAL];

    if (argc < 3) {
        printf("Argumentos insuficientes. -<SIGNAL> (Señal a ejecutar, valor númerico) <PID> (proceso).\n");
        return(EXIT_FAILURE);
    }

    /* Comprobando el '-' */
    if (argv[1][0] != '-') {
        printf("Debes introducir el '-'.\n");
        return(EXIT_FAILURE);   
    }

    /* Inicializando la String SIGNAL */
    for (int i = 0; i < MAXLENSIGNAL; i++) SIGNAL[i] = '\0';

    /* Copiando el SIGNAL sin el '-' */
    for (int i = 0; argv[1][i+1] != '\0'; i++) SIGNAL[i] = argv[1][i+1];

    /* Mandando la señal */
    kill(atoi(argv[2]), atoi(SIGNAL));

    return 0;
}
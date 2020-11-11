/**
 * @file estructura.h
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Archivo donde se guardan nombres de semaforos, memoria compartida 
 * y las estructuras utilizadas en los programas shm_producer_file.c shm_consumer_file.c
 * shm_producer.c shm_consumer.c
 * @version 1.0
 * @date 2020-04-8
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#ifndef ESTRUCTURA_H
#define ESTRUCTURA_H

#define SHM_NAME "/shm_lista"
#define SEM_NAME "/empty"
#define SEM_NAME1 "/mutex"
#define SEM_NAME2 "/fill"
#define SEM_NAME3 "/exec"
#define SIZE 10
#define FILENAME "fichero.txt"

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <signal.h>
#include <semaphore.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

/**
 * @brief Estructura diseñada para ser compartida
 * 
 */
typedef struct {
    sem_t *exec;            //Semaforo para dejar ejecutarse al consumidor
    sem_t *sem_empty;
    sem_t *sem_mutex;
    sem_t *sem_fill;         
    int lista[SIZE];        //Lista con los enteros
    int indice;             //Indice 
} memoria_compartida;

/**
 * @brief Estructura para la parte opcional del ejercicio 4
 * 
 */
typedef struct {
    sem_t *exec;            //Semaforo para dejar ejecutarse al consumidor
    sem_t *sem_empty;
    sem_t *sem_mutex;
    sem_t *sem_fill;         
    int indice;             //Indice 
} memoria_compartida_fichero;

/**
 * @brief Función para que un proceso haga un mapeado de la memoria compartida
 * y ejecutar el algoritmo del consumidor
 * 
 */
void consumidor();

/**
 * @brief Función para que un proceso haga un mapeado de la memoria compartida
 * y ejecutar el algoritmo del consumidor usando un archivo
 * 
 */
void consumidor_file();

#endif
/**
 * @file shm_consumer.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Archivo que se encarga de simular la accion de consumir en el 
 * algoritmo del productor-consumidor, en este caso se usa un array con
 * memoria compartida en el cual el consumidor se encarga de contar los 
 * numeros que aparecen
 * @version 1
 * @date 2020-04-6
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "estructura.h"

/**
 * @brief Función para que un proceso haga un mapeado de la memoria compartida
 * y ejecutar el algoritmo del consumidor
 * 
 * @param N Numero de elementos a leer
 */
void consumidor() {

    int histograma[SIZE], leido;

    /* Inicializando el array */
    for (int i = 0; i < SIZE; i++) histograma[i] = 0;

    /* Abrimos la memoria compartida */
    int fd_shm = shm_open(SHM_NAME, O_RDONLY, 0);

    if (fd_shm == -1) {
        fprintf(stderr, "Error abriendo la memoria compartida\n");
        return;
    }

    /* Mapeando el segmento de memoria */
    memoria_compartida *mem = mmap(NULL, sizeof(*mem),
        PROT_READ,
        MAP_SHARED,
        fd_shm,
        0);
    close(fd_shm);

    if (mem == MAP_FAILED) {
        fprintf(stderr, "Error mapping the shared memory segment \n");
        return;
    }

    /* Hacemos down al semaforo para ejecutar el algoritmo del consumidor */
    sem_wait(mem->exec);

    while(1) {
        sem_wait(mem->sem_fill);
        sem_wait(mem->sem_mutex);

        /* Esto simularia la extraccion del elemento */
        leido = mem->lista[mem->indice];
        if (leido == -1) {
            sem_post(mem->sem_mutex);
            sem_post(mem->sem_empty);
            break;
        }
        histograma[leido] += 1;

        sem_post(mem->sem_mutex);
        sem_post(mem->sem_empty);
    }
    
    for (int n = 0; n < SIZE; n++) printf("Numero: %d. Veces leidas: %d\n", n, histograma[n]);

    /* Dejamos al productor liberar recursos */
    sem_post(mem->exec);

    munmap(mem, sizeof(*mem));

    exit(EXIT_SUCCESS);
}


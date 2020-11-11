/**
 * @file shm_consumer_file.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Este archivo es casi identico a shm_consumer la diferencia
 * es que este hace la misma funcion pero usando un fichero de texto
 * @version 1
 * @date 2020-04-6
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "estructura.h"

/* Hemos hecho esta variable global para poder utilizarla en el manejador */
memoria_compartida_fichero *mem_consumidor = NULL;

/**
 * @brief Manejador de la señal SIGUSR2
 * 
 * @param sig Signal
 */
void manejador_SIGUSR2(int sig) {
    munmap(mem_consumidor, sizeof(*mem_consumidor));
    kill(getppid(), SIGUSR2);
    exit(EXIT_FAILURE);
}

/**
 * @brief Función para que un proceso haga un mapeado de la memoria compartida
 * y ejecutar el algoritmo del consumidor
 * 
 * @param N Numero de elementos a leer
 */
void consumidor_file() {

    struct sigaction mask_consumidor; /* Mascara para el control de errores */
    int histograma[SIZE], leido[SIZE];
    FILE *pf = NULL;

    /* Creando mascaras */
    sigfillset(&(mask_consumidor.sa_mask));
    mask_consumidor.sa_flags = 0;

    /* Eliminamos la unica señal que esperamos */
    sigdelset(&(mask_consumidor.sa_mask), SIGUSR2);

    if (sigprocmask(SIG_SETMASK, &(mask_consumidor.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        kill(getppid(), SIGUSR2);
    }

    /* Estableciendo manejador */
    mask_consumidor.sa_handler = manejador_SIGUSR2;
    if (sigaction(SIGALRM, &mask_consumidor, NULL) < 0) {
        perror("sigaction");
        kill(getppid(), SIGUSR2);
    }

    /* Inicializando el array */
    for (int i = 0; i < SIZE; i++) histograma[i] = 0;

    /* Abrimos la memoria compartida */
    int fd_shm = shm_open(SHM_NAME, O_RDONLY, 0);

    if (fd_shm == -1) {
        fprintf(stderr, "Error abriendo la memoria compartida\n");
        kill(getppid(), SIGUSR2);
        return;
    }

    /* Mapeando el segmento de memoria */
    mem_consumidor = mmap(NULL, sizeof(*mem_consumidor),
        PROT_READ,
        MAP_SHARED,
        fd_shm,
        0);
    close(fd_shm);

    if (mem_consumidor == MAP_FAILED) {
        fprintf(stderr, "Error mapping the shared memory segment \n");
        kill(getppid(), SIGUSR2);
        return;
    }

    /* Hacemos down al semaforo para ejecutar el algoritmo del consumidor */
    sem_wait(mem_consumidor->exec);

    while(1) {
        sem_wait(mem_consumidor->sem_fill);
        sem_wait(mem_consumidor->sem_mutex);

        /* Abrimos el archivo */
        pf = fopen("fichero.txt", "r+");
        if (pf == NULL) {
            perror("sem_open");
            kill(getppid(), SIGUSR2);
            munmap(mem_consumidor, sizeof(*mem_consumidor));
            exit(EXIT_FAILURE);
        }
        
        /* Leemos la lista */
        for (int n = 0; n < SIZE; n++) fscanf(pf, "%d ", &leido[n]);

        /* Esto simularia la extraccion del elemento */
        if (leido[mem_consumidor->indice] == -1) {
            sem_post(mem_consumidor->sem_mutex);
            sem_post(mem_consumidor->sem_empty);
            break;
        }
        histograma[leido[mem_consumidor->indice]] += 1;

        /* Cerramos el archivo */
        fclose(pf);

        sem_post(mem_consumidor->sem_mutex);
        sem_post(mem_consumidor->sem_empty);
    }

    sem_post(mem_consumidor->exec);
    
    for (int n = 0; n < SIZE; n++) printf("Numero: %d. Veces leidas: %d\n", n, histograma[n]);

    munmap(mem_consumidor, sizeof(*mem_consumidor));

    exit(EXIT_SUCCESS);
}


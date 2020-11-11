/**
 * @file shm_producer.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Archivo donde se simula la accion del productor en el algoritmo
 * productor-consumidor, en este archivo se usa memoria compartida y se escriben
 * numeros del 0 al 9 de manera aleatoria o secuencial
 * @version 1
 * @date 2020-04-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include "estructura.h"

/**
 * @brief Funcion que simula el comportamiento del productor
 * 
 * @param N Cantidad de números a generar
 * @param mode Manera en la que se generan los numeros (aleatorio o secuencia) 
 * @param mem Memoria compartida
 */
void productor(int N, int mode, memoria_compartida *mem) {

    /* Dejamos al consumidor ejecutarse */
    sem_post(mem->exec);

    /* Bucle para introducir los numeros en secuencia */
    for(int i = 0; i < N; i++) {

        sem_wait(mem->sem_empty);
        sem_wait(mem->sem_mutex);

        /* Esto corresponderia a "añadir elemento" */
        mem->indice = (mem->indice + 1)%SIZE;
        if (i != N - 1 && mode == 1) mem->lista[mem->indice] = i%10;
        else if (i != N - 1 && mode == 0) mem->lista[mem->indice] = rand()%10;
        else mem->lista[mem->indice] = -1;

        sem_post(mem->sem_mutex);
        sem_post(mem->sem_fill);
    }
}

int main(int argc, char **argv) {

    int N, mode;
    pid_t pid;

    srand (time(NULL));

    /* Comprobamos argumentos */
    if (argc < 3) {
        printf("Argumentos insuficientes, recuerde <N> <Modo (0 aleatorio, 1 secuencia)>\n");
        return EXIT_FAILURE;
    }

    N = atoi(argv[1]);
    if (N < 0) {
        printf("Introducir digitos, recuerde <N> <Modo (0 aleatorio, 1 secuencia)>\n");
        return EXIT_FAILURE;
    }

    mode = atoi(argv[2]);
    if (mode < 0 || mode > 1) {
        printf("Introducir digitos, recuerde <N> <Modo (0 aleatorio, 1 secuencia)>\n");
        return EXIT_FAILURE;
    }


    /* Creamos la memoria compartida */
    int fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

    if (fd_shm == -1) {
        fprintf(stderr, "Error creando la memoria compartida\n");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    /* Cambiamos el tamaño de la memoria */
    if (ftruncate(fd_shm, sizeof(memoria_compartida)) == -1) {
        fprintf(stderr, "Error cambiando el tamaño de la memoria compartida\n");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    /* Mapeamos la memoria compartida en un puntero que vayamos a usar */
    memoria_compartida *mem = mmap(NULL,  /* El kernel elige la direccion donde crear el mapeado */
    sizeof(*mem),
    PROT_READ | PROT_WRITE,               /* Hacemos posible la lectura y escritura */
    MAP_SHARED,                           /* Compartimos el mapeado, las actualizaciones de este son visibles para otros procesos */
    fd_shm, 0);                           /* Descriptor de fichero */

    close(fd_shm); /* Cerramos el descriptor de fichero */

    if (mem == MAP_FAILED) {
        fprintf(stderr, "Error mapeando la memoria compartida\n");
        shm_unlink(SHM_NAME);
    }

    /* Creando los semaforos */
    if ((mem->sem_empty = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
		perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
	}

    /* Creando los semaforos */
    if ((mem->sem_mutex = sem_open(SEM_NAME1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
		perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
	}

    /* Creando los semaforos */
    if ((mem->sem_fill = sem_open(SEM_NAME2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
	}

    /* Creando los semaforos */
    if ((mem->exec = sem_open(SEM_NAME3, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
	}

    /* Inicializando la lista */
    for(int i = 0; i < 9; i++) mem->lista[i] = 0;

    /* El hijo comienza a ejecutarse para simular el comportamiento del consumidor */
    /* Se comienza a ejecutar despues de haber creado la memoria para que no ocurra el caso en el
    que el consumidor trate de abrir la memoria antes de que se haya creado. Aun asi el hijo busca la memoria compartida */
    pid = fork();
    if (pid == 0) {
        consumidor();
    }

    productor(N, mode, mem);

    /* Hasta que el consumidor no haya terminado de consumir no liberamos */
    sem_wait(mem->exec);
    sem_close(mem->exec);
    sem_close(mem->sem_empty);
    sem_close(mem->sem_fill);
    sem_close(mem->sem_mutex);

    /* Liberando la memoria compartida y semaforos */
    munmap(mem, sizeof(*mem));
    sem_unlink(SEM_NAME);
    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    sem_unlink(SEM_NAME3);
    shm_unlink(SHM_NAME);  

    return EXIT_SUCCESS; 
}


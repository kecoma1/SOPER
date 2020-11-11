/**
 * @file shm_producer_file.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Archivo casi identico que shm_producer, en este se utiliza un archivo
 * en vez de memoria compartida.
 * @version 1
 * @date 2020-04-11
 * 
 * @copyright Copyright (c) 2020
 */
#include "estructura.h"

/* Hemos hecho esta variable global para poder utilizarla en el manejador */
memoria_compartida_fichero *mem_productor = NULL;
struct sigaction mask_productor; /* Mascara para el control de errores */

/**
 * @brief Manejador de la señal SIGUSR2
 * 
 * @param sig Señal
 */
void manejador_SIGUSR2_productor(int sig) {
    sem_close(mem_productor->exec);
    sem_close(mem_productor->sem_empty);
    sem_close(mem_productor->sem_fill);
    sem_close(mem_productor->sem_mutex);
    shm_unlink(SHM_NAME);
    sem_unlink(SEM_NAME);
    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    sem_unlink(SEM_NAME3);
    exit(EXIT_FAILURE);
}

/**
 * @brief Funcion que simula el comportamiento del productor
 * 
 * @param N Cantidad de números a generar
 * @param mode Manera en la que se generan los numeros (aleatorio o secuencia) 
 * @param mem Memoria compartida
 */
void productor_file(int N, int mode, memoria_compartida_fichero *mem, pid_t consumer) {

    FILE *pf = NULL;
    int lista[SIZE]; /* Aqui se guarda la lista cada vez que producimos */

    /* Dejamos al consumidor ejecutarse */
    sem_post(mem->exec);

    /* Bucle para introducir los numeros en secuencia */
    for(int i = 0; i < N; i++) {

        sem_wait(mem->sem_empty);
        sem_wait(mem->sem_mutex);

        /* Abrimos el archivo */
        pf = fopen("fichero.txt", "r+");
        if (pf == NULL) {
            perror("sem_open");
            kill(consumer, SIGUSR2);
            sigsuspend(&(mask_productor.sa_mask)); /* Esperamos a que el hijo termine su ejecución y libere sus recursos */                
        }
        
        /* Leemos la lista */
        for (int n = 0; n < SIZE; n++) fscanf(pf, "%d ", &lista[n]);

        /* Esto corresponderia a "añadir elemento" */
        mem->indice = (mem->indice + 1)%SIZE;                       
        if (i != N - 1 && mode == 1) lista[mem->indice] = i%10;             /* Modo secuencia */
        else if (i != N -1 && mode == 0) lista[mem->indice] = rand()%10;    /* Modo aleatorio */
        else lista[mem->indice] = -1;

        /* Volvemos a escribir en el archivo */
        fseek(pf, 0, SEEK_SET);
        for (int n = 0; n < SIZE; n++) fprintf(pf, "%d ", lista[n]);

        /* Cerramos el archivo */
        fclose(pf);
        sem_post(mem->sem_mutex);
        sem_post(mem->sem_fill);
    } 
}

int main(int argc, char **argv) {

    int N, mode;
    pid_t pid;
    FILE *pf;

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

    /* Creando mascaras */
    sigfillset(&(mask_productor.sa_mask));
    mask_productor.sa_flags = 0;

    /* Eliminamos la única señal que esperamos */
    sigdelset(&(mask_productor.sa_mask), SIGUSR2);

    if (sigprocmask(SIG_SETMASK, &(mask_productor.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        return(EXIT_FAILURE);
    }

    /* Estableciendo manejador */
    mask_productor.sa_handler = manejador_SIGUSR2_productor;
    if (sigaction(SIGALRM, &mask_productor, NULL) < 0) {
        perror("sigaction");
        return(EXIT_FAILURE);
    }

    /* Creamos la memoria compartida */
    int fd_shm = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);

    if (fd_shm == -1) {
        fprintf(stderr, "Error creando la memoria compartida\n");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    /* Cambiamos el tamaño de la memoria */
    if (ftruncate(fd_shm, sizeof(memoria_compartida_fichero)) == -1) {
        fprintf(stderr, "Error cambiando el tamaño de la memoria compartida\n");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

    /* Mapeamos la memoria compartida en un puntero que vayamos a usar */
    mem_productor =  mmap(NULL,  /* El kernel elige la direccion donde crear el mapeado */
    sizeof(*mem_productor),
    PROT_READ | PROT_WRITE,               /* Hacemos posible la lectura y escritura */
    MAP_SHARED,                           /* Compartimos el mapeado, las actualizaciones de este son visibles para otros procesos */
    fd_shm, 0);                           /* Descriptor de fichero */

    close(fd_shm); /* Cerramos el descriptor de fichero */

    if (mem_productor == MAP_FAILED) {
        fprintf(stderr, "Error mapeando la memoria compartida\n");
        shm_unlink(SHM_NAME);
    }

    /* Creando los semaforos */
    if ((mem_productor->sem_empty = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
		perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
	}

    /* Creando los semaforos */
    if ((mem_productor->sem_mutex = sem_open(SEM_NAME1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
		perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
	}

    /* Creando los semaforos */
    if ((mem_productor->sem_fill = sem_open(SEM_NAME2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
	}

    /* Creando los semaforos */
    if ((mem_productor->exec = sem_open(SEM_NAME3, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		perror("sem_open");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
	}

    /* Inicializando el archivo */
    pf = fopen("fichero.txt", "w+");
    if (pf == NULL) {
        perror("fopen");
        shm_unlink(SHM_NAME);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        sem_unlink(SEM_NAME3);
		exit(EXIT_FAILURE);
    }
    /* Inicializando la lista */
    for (int i = 0; i < SIZE; i++) fprintf(pf, "%d ",0);
    fclose(pf);

    /* El hijo comienza a ejecutarse para simular el comportamiento del consumidor */
    /* Se comienza a ejecutar despues de haber creado la memoria para que no ocurra el caso en el
    que el consumidor trate de abrir la memoria antes de que se haya creado. Aun asi el hijo busca la memoria compartida */
    pid = fork();
    if (pid == 0) {
        consumidor_file();
    }

    productor_file(N, mode, mem_productor, pid);

    /* Hasta que el consumidor no haya terminado de consumir no liberamos */
    sem_wait(mem_productor->exec);

    /* Liberando la memoria compartida y semaforos */
    sem_close(mem_productor->exec);
    sem_close(mem_productor->sem_empty);
    sem_close(mem_productor->sem_fill);
    sem_close(mem_productor->sem_mutex);
    munmap(mem_productor, sizeof(*mem_productor));
    sem_unlink(SEM_NAME);
    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    sem_unlink(SEM_NAME3);
    shm_unlink(SHM_NAME);  

    return EXIT_SUCCESS; 
}


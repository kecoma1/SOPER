#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "sort.h"
#include "utils.h"

#define SHM_NAME "/shm"
#define VAR_AUX "/shm_aux"
#define SEM_NAME "/sem_1"
#define SEM_NAME1 "/sem_2"
#define COLA_MEN "/cola_de_mensajes"

/**
 * @brief Estructura que contiene un contador usado
 * por el proceso principal y los procesos trabajadores
 * 
 */
typedef struct {
    int contador;
} compartido;

/**
 * @brief Manejador para la señal SIGTERM
 * El proceso muere liberando los recursos utilizados 
 * 
 * @param sig 
 */
void manejador_SIGTERM(int sig) {

}

/**
 * @brief Manejador para la señal SIGUSR1
 * 
 * @param sig 
 */
void manejador_SIGUSR1(int sig) {

}

int main(int argc, char *argv[]) {
    /* Estructura utilizada por la cola de mensajes */
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_curmsgs = 0,
        .mq_msgsize = sizeof(Task)
    };

    /*int n_levels, n_processes, delay;*/
    int shmid;
    int shm_aux;
    pid_t pid;
    sem_t *sem = NULL, *sem1 = NULL;
    Sort *sort_comp = NULL;
    compartido *contador = NULL;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <FILE> <N_LEVELS> <N_PROCESSES> [<DELAY>]\n", argv[0]);
        fprintf(stderr, "    <FILE> :        Data file\n");
        fprintf(stderr, "    <N_LEVELS> :    Number of levels (1 - %d)\n", MAX_LEVELS);
        fprintf(stderr, "    <N_PROCESSES> : Number of processes (1 - %d)\n", MAX_PARTS);
        fprintf(stderr, "    [<DELAY>] :     Delay (ms)\n");
        exit(EXIT_FAILURE);
    }

    /* Inicializamos la cola de mensajes */
    mqd_t cola = mq_open(COLA_MEN, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);
    if ((mqd_t) -1 == cola) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    /* Creamos un semaforo con valor igual a las tareas del nivel */
    if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
        perror("sem_open");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        exit(EXIT_FAILURE);
    }

    /* Creamos un semaforo con valor igual a las tareas del nivel */
    if ((sem1 = sem_open(SEM_NAME1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
        perror("sem_open");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_close(sem);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Abriendo la memoria compartida */
    shmid = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (shmid == -1) {
		perror("shm_open did not created the shared memory correctly.");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_close(sem);
        sem_close(sem1);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Creamos memoria compartida para el contador auxiliar */
    shm_aux = shm_open(VAR_AUX, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (shm_aux == -1) {
		perror("shm_open did not created the shared memory correctly.");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_close(sem);
        sem_close(sem1);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Cambiando el tamaño de la memoria compartida */
    if (ftruncate(shmid, sizeof(Sort)) == -1) {
		perror("ftruncate did not perform successfully.");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_close(sem);
        sem_close(sem1);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
        close(shmid);
        close(shm_aux);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        exit(EXIT_FAILURE);
	}

    /* Cambiamos el tamaño de la memoria compartida auxiliar */
    if (ftruncate(shm_aux, sizeof(compartido)) == -1) {
		perror("ftruncate did not perform successfully.");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_close(sem);
        sem_close(sem1);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
        close(shmid);
        close(shm_aux);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        exit(EXIT_FAILURE);
	}

    /* Hacemos el mapeo de la memoria. */
    sort_comp = mmap(NULL, sizeof(*sort_comp), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
	if(!sort_comp){
		perror("mmap: ");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_close(sem);
        sem_close(sem1);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
        close(shmid);
        close(shm_aux);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        exit(EXIT_FAILURE);
	}

    /* Hacemos el mapeo de la memoria compartida auxiliar. */
    contador = mmap(NULL, sizeof(*contador), PROT_READ | PROT_WRITE, MAP_SHARED, shm_aux, 0);
	if(!contador){
		perror("mmap: ");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_close(sem);
        sem_close(sem1);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
        munmap(sort_comp, sizeof(*sort_comp));
        close(shmid);
        close(shm_aux);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        exit(EXIT_FAILURE);
	}

    /* Inicializando la memoria compartida */
    if (init_sort(argv[1], sort_comp, atoi(argv[2]), atoi(argv[3]),atoi(argv[4])) == ERROR) {
        fprintf(stderr, "sort_single_process (shared memory) - init_sort\n");
        mq_close(cola);
        mq_unlink(COLA_MEN);
        sem_close(sem);
        sem_close(sem1);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
        munmap(sort_comp, sizeof(*sort_comp));
        munmap(contador, sizeof(*contador));
        close(shmid);
        close(shm_aux);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        exit(EXIT_FAILURE);
    } 

    if (argc > 4) {
        sort_comp->delay = 1e6 * atoi(argv[4]);
    }
    else {
        sort_comp->delay = 1e8;
    }

    /* Bucle para crear procesos por cada nivel */
    for (int i = 0; i < sort_comp->n_levels; i++) {

        /* Bucle para cada tarea del nivel */
        for (int n = 0; n < get_number_parts(i, sort_comp->n_levels); n++) {
            
            /* Inicializamos el valor de contador */
            sem_wait(sem1);
            contador->contador = get_number_parts(i, sort_comp->n_levels);
            sem_post(sem1);

            /* El padre hace fork */
            pid = fork();
            if (pid == 0) {

                Task tarea_recibida;

                /* Cola creada para que los trabajadores solo reciban mensajes de la cola */
                mqd_t queue = mq_open(COLA_MEN, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attributes);
                if(queue == (mqd_t)-1) {
                    fprintf(stderr, "Error opening the queue\n");
                    sem_close(sem);
                    sem_close(sem1);
                    munmap(sort_comp, sizeof(*sort_comp));
                    close(shmid);
                    return EXIT_FAILURE;
                }

                /* Los trabajadores reciben los mensajes */
                if (mq_receive(queue, (char *)&tarea_recibida, sizeof(Task), NULL) == -1) {
                    fprintf(stderr, "Error receiving message\n");
                    mq_close(cola);
                    sem_close(sem);
                    sem_close(sem1);
                    munmap(sort_comp, sizeof(*sort_comp));
                    close(shmid);
                    exit(EXIT_FAILURE);
                }

                /* Se le asigna una tarea al proceso hijo (trabajador)
                 y este sale del bucle */
                if (solve_task_by_task(sort_comp, &tarea_recibida) == ERROR) {
                    mq_close(cola);
                    sem_close(sem);
                    sem_close(sem1);
                    munmap(sort_comp, sizeof(*sort_comp));
                    close(shmid);
                    exit(EXIT_FAILURE);
                }

                /* El proceso hijo accede a la memoria compartida */
                sem_wait(sem1);
                /* Comprobamos que este proceso sea el último en ejecutarse para así
                dejar al padre continuar con el siguiente nivel */
                contador->contador -= 1;
                if (contador->contador == 0) sem_post(sem);
                sem_post(sem1);

                /* El trabajador abandona su ejecución */
                mq_close(queue);
                break;
            } else if (pid == -1) {
                sem_close(sem);
                sem_close(sem1);
                mq_close(cola);
                munmap(sort_comp, sizeof(*sort_comp));
                close(shmid);
                exit(EXIT_FAILURE);
            }
        }

        /* El trabajador sale del primer loop */
        if (pid == 0) break;

        /* Metemos en la cola de mensajes todas las tareas del nivel */
        for (int k = 0; k < get_number_parts(i, sort_comp->n_levels); k++) {
            if (mq_send(cola, (char *) &sort_comp->tasks[i][k], sizeof(Task), 1) == -1) {
                fprintf(stderr, "Error mandando el mensaje\n");
                perror("mq_send");
                mq_close(cola);
                sem_close(sem);
                sem_close(sem1);
                munmap(sort_comp, sizeof(*sort_comp));
                munmap(contador, sizeof(*contador));
                close(shmid);
                mq_unlink(COLA_MEN);
                shm_unlink(SHM_NAME); 
                shm_unlink(VAR_AUX);
                sem_unlink(SEM_NAME1);
                sem_unlink(SEM_NAME);
                exit(EXIT_FAILURE);
            }
        }

        /* El padre espera a que el último hijo acabe la tarea */
        sem_wait(sem);
        while(wait(NULL) != -1);
        plot_vector(sort_comp->data, sort_comp->n_elements);
    }

    if (pid != 0) {
        for(int i = 0; i < 32; i++)printf("%d --", sort_comp->data[i]);/*------------------------CACA---------------------------------*/
        printf("\n");
    }
    munmap(sort_comp, sizeof(*sort_comp));
    close(shmid);
    sem_close(sem);
    sem_close(sem1);
    mq_close(cola);
    
    /* El padre es el único en hacer unling */
    if (pid != 0) {
        mq_unlink(COLA_MEN);
        shm_unlink(VAR_AUX);
        shm_unlink(SHM_NAME); 
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME);
    }
    return EXIT_SUCCESS;
}

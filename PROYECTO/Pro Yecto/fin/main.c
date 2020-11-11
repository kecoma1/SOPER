/**
 * @file main.c
 * 
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * 
 * @brief Archivo donde se ejecuta el programa para ordenar de manera 
 * gráfica un array usando bubblesort y mergesort. En este archivo se usa
 * un sistema multiproceso para ordenar el array de forma concurrente
 * 
 * @version 7.0
 * 
 * @date 2020-05-06
 * 
 * @copyright Copyright (c) 2020
 * 
 */
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
#define SEM_NAME2 "/sem_3"
#define COLA_MEN "/cola_de_mensajes"

/*************************** Estructuras *****************************/
/**
 * @brief Estructura que contiene un contador usado
 * por el proceso principal y los procesos trabajadores
 * 
 * Este contador es usado para saber las tareas restantes por cada nivel,
 * debe ser usada como una variable compartida. Cada vez que un proceso termina 
 * su tarea este decrementa el contador, el padre cuando han terminado cambia el
 * valor de esta variable y le pone el numero de tareas del nivel a resolver
 * 
 */
typedef struct {
    int contador;           /* Variable usada como contador */
} compartido;

/**
 * @brief Estructura que define un mensaje
 * 
 */
typedef struct {
    Task *tarea_enviada;    /* Puntero a la tarea enviada, (apunta a la memoria compartida) */
} mensaje;

/**
 * @brief Enumeración para que el hijo sepa que esta haciendo
 * 
 */
typedef enum {
    ESPERANDO, ORDENANDO
} estado;

/*************************** Variables globales *****************************/

mqd_t cola = -1;                                    /* Cola de mensajes utilizada por el proceso principal */
mqd_t queue = -1;                                   /* Cola de mensajes utilizada por los trabajadores */
sem_t *sem = NULL,                                  /* Semáforo usado para resolver los niveles en orden  */
      *sem1 = NULL,                                 /* Semáforo usado para acceder a la variable compartida "contador"  */
      *sem2 = NULL;                                 /* Semáforo usado para cambiar el estado de las tareas */
Sort *sort_comp = NULL;                             /* Estructura pricipal compartida */
int shmid = -1;                                     /* Descriptor de fichero de la memoria compartida */
pid_t *hijos = NULL;                                /* Puntero donde el padre guarda los pids de los hijos */
int indice_tuberia;                                 /* Variable donde al crear el proceso este tiene guardado el indice que le indica cual es la tuberia a la que acceder,
                                                     esta variable no la modifica nadie excepto el padre, por lo tanto los hijos siempre van a tener la misma */
int state;                                          /* Variable unicamente utilizada por los hijos, estos la modifican cada vez que ordenan o esperan.
                                                     si la variable vale 0, esque el proceso esta esperando, si la variable es > 0 es que esta ordenando.
                                                     En concreto el primer digito sera para mostrar si hace bublesort (1) o mergesort (2), los 4 siguientes
                                                     digitos seran para el primer indice de la tarea y los cuatro ultimos para el segundo indice 
                                                     Ejemplo: ini = 4, end = 7, bubblesort.   state = 100040007*/

/*************************** Manejadores *****************************/
/**
 * @brief Manejador para la señal SIGTERM
 * El proceso muere liberando los recursos utilizados 
 * 
 * @param sig Señal recibida
 */
void manejador_SIGTERM(int sig) {
    if (hijos != NULL) free(hijos);
    if (sort_comp != NULL) munmap(sort_comp, sizeof(*sort_comp));
    if (shmid != -1) close(shmid);
    if (sem != NULL) sem_close(sem);
    if (sem1 != NULL) sem_close(sem1);
    if (sem2 != NULL) sem_close(sem2);
    if (queue != -1) {
        mq_close(queue);
    }

    exit(EXIT_SUCCESS);
}

/**
 * @brief Manejador para la señal SIGUSR1
 * 
 * @param sig Señal recibida
 */
void manejador_SIGUSR1(int sig) {
}

/**
 * @brief Manejador para la señal SIGINT
 * 
 * @param sig Señal recibida
 */
void manejador_SIGINT(int sig) {
    /* Envia la señal SIGTERM a los hijos */
    for(int i = 0; i < sort_comp->n_processes+2; i++) {
        kill(hijos[i], SIGTERM);
    }

    /* Liberando los recursos del padre */
    if (sort_comp != NULL) munmap(sort_comp, sizeof(*sort_comp));
    if (shmid != -1) close(shmid);
    if (sem != NULL)sem_close(sem);
    if (sem1 != NULL) sem_close(sem1);
    if (sem2 != NULL) sem_close(sem2);
    if (cola != -1) mq_close(cola);
    if (hijos != NULL) free(hijos);
    mq_unlink(COLA_MEN);
    shm_unlink(VAR_AUX);
    shm_unlink(SHM_NAME); 
    sem_unlink(SEM_NAME);
    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    exit(EXIT_SUCCESS);
}

/**
 * @brief Manejador para la señal SIGALRM
 * Aquí es donde el trabajador le manda lo que esta haciendo al ilustrador
 * 
 * El trabajador escribe en la tuberia "indice_tuberia"*2, el ilustrador escribe en
 * las tuberias impares 
 * 
 * @param sig Señal recibida
 */
void manejador_SIGALRM(int sig) {
    printf("%d estoy en el manejador\n", getpid());
}

int main(int argc, char *argv[]) {
    /* Estructura utilizada por la cola de mensajes */
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_curmsgs = 0,
        .mq_msgsize = sizeof(mensaje)
    };

    int shm_aux;                        /* Variable usada para el descriptor de fichero auxiliar de la memoria compartida */
    int contador_p_restantes = 0;       /* Varaible donde se guarda el valor de la variable de memoria compartida "contador" */
    Bool boolean = FALSE;               /* Variable booleana usada para comprobar el estado de las tareas de un nivel */
    pid_t pid;                          /* Variable donde los hijos guardan 0 al ser creados, el padre guarda != 0 */
    mensaje msg[MAX_PARTS];             /* Variable usada para enviar mensajes, es un array con un tamaño maximo igual que el de las tareas,
                                         por lo tanto nunca va a haber más tareas que "sobres" donde se envien los mensajes  */
    compartido *contador = NULL;        /* Puntero a la direccion de memoria de la variable compartida usada como contador de las tareas restantes por nivel */
    struct sigaction padreMask;         /* Mascara del padre, ignora todas las señales excepto SIGUSR1 */
    struct sigaction padreMask_SI;      /* Segunda mascara del padre, ignora todas las señales excepto SIGINT */
    struct sigaction hijoMask;          /* Mascara de los hijos, ignoran todas las señales excepto SIGTERM (también es utilizada por el ilustrador) */
    struct sigaction hijoMask_SA;       /* Segunda mascara del hijo, ignora todas las señales excepto SIGALRM */
    struct itimerval timer;             /* Temporizador para los trabajadores */

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <FILE> <N_LEVELS> <N_PROCESSES> [<DELAY>]\n", argv[0]);
        fprintf(stderr, "    <FILE> :        Data file\n");
        fprintf(stderr, "    <N_LEVELS> :    Number of levels (1 - %d)\n", MAX_LEVELS);
        fprintf(stderr, "    <N_PROCESSES> : Number of processes (1 - %d)\n", MAX_PARTS);
        fprintf(stderr, "    [<DELAY>] :     Delay (ms)\n");
        exit(EXIT_FAILURE);
    }

    /* Inicializando la timer */
    timer.it_interval.tv_sec = 1;
    timer.it_interval.tv_usec = 0;
    timer.it_value.tv_sec = 1;
    timer.it_value.tv_usec = 0;
    setitimer(ITIMER_REAL, &timer, NULL);

    /* Inicializamos la cola de mensajes */
    cola = mq_open(COLA_MEN, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR, &attributes);
    if ((mqd_t) -1 == cola) {
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    /* Creamos un semaforo con valor igual a las tareas del nivel */
    if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
        perror("sem_open");

        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        mq_unlink(COLA_MEN);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        exit(EXIT_FAILURE);
    }

    /* Creamos un semaforo con valor igual a las tareas del nivel */
    if ((sem1 = sem_open(SEM_NAME1, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
        perror("sem_open");

        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        sem_close(sem);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        mq_unlink(COLA_MEN);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        exit(EXIT_FAILURE);
    }

    /* Semaforo para controlar el estado de las tareas  */
    if ((sem2 = sem_open(SEM_NAME2, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
        perror("sem_open");

        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        sem_close(sem);
        sem_close(sem1);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        mq_unlink(COLA_MEN);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        exit(EXIT_FAILURE);
    }

    /* Abriendo la memoria compartida */
    shmid = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (shmid == -1) {
		perror("shm_open");
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        mq_unlink(COLA_MEN);
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        exit(EXIT_FAILURE);
    }

    /* Creamos memoria compartida para el contador auxiliar */
    shm_aux = shm_open(VAR_AUX, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (shm_aux == -1) {
		perror("shm_open");
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);
        
        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
    }

    /* Cambiando el tamaño de la memoria compartida */
    if (ftruncate(shmid, sizeof(Sort)) == -1) {
		perror("ftruncate");
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
	}

    /* Cambiamos el tamaño de la memoria compartida auxiliar */
    if (ftruncate(shm_aux, sizeof(compartido)) == -1) {
		perror("ftruncate");
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
	}

    /* Hacemos el mapeo de la memoria. */
    sort_comp = mmap(NULL, sizeof(*sort_comp), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
	if(!sort_comp){
		perror("mmap");
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
	}

    /* Hacemos el mapeo de la memoria compartida auxiliar. */
    contador = mmap(NULL, sizeof(*contador), PROT_READ | PROT_WRITE, MAP_SHARED, shm_aux, 0);
	if(!contador){
		perror("mmap: ");

        /* Hacemos unmap de las varaibles ya mapeadas */
        munmap(sort_comp, sizeof(*sort_comp));
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
	}
    /* Iniciailizamos la variable */
    contador->contador = 0;

    /* Inicializando la memoria compartida */
    if (init_sort(argv[1], sort_comp, atoi(argv[2]), atoi(argv[3]),atoi(argv[4])) == ERROR) {
        fprintf(stderr, "Error al ejecutar init_sort.\n");

        /* Hacemos unmap de las varaibles ya mapeadas */
        munmap(sort_comp, sizeof(*sort_comp));
        munmap(contador, sizeof(*contador));
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
    } 

    /* Creando mascaras */
    sigfillset(&(padreMask.sa_mask));
    padreMask.sa_flags = 0;

    sigfillset(&(hijoMask.sa_mask));
    hijoMask.sa_flags = 0;

    sigfillset(&(padreMask_SI.sa_mask));
    padreMask_SI.sa_flags = 0;

    sigfillset(&(hijoMask_SA.sa_mask));
    hijoMask_SA.sa_flags = 0;

    /* El proceso padre esperan la señal SIGUSR1,
     la eliminamos de la mascara */
    sigdelset(&(padreMask.sa_mask), SIGUSR1);

    /* Los procesos hijo esperan la señal SIGTERM y SIGALRM,
     las eliminamos de la mascara */
    sigdelset(&(hijoMask.sa_mask), SIGTERM);

    /* El proceso padre tiene otra mascara para 
     recibir la señal SIGINT */
    sigdelset(&(padreMask_SI.sa_mask), SIGINT);

    /* El proceso hijo tiene otra mascara para
     recibir la señal SIGALRM */
    sigdelset(&(hijoMask_SA.sa_mask), SIGALRM);
    sigdelset(&(hijoMask_SA.sa_mask), SIGTERM);

    /* Estableciendo manejadores */
    padreMask.sa_handler = manejador_SIGUSR1;
    if (sigaction(SIGUSR1, &padreMask, NULL) < 0) {
        perror("sigaction");

        /* Hacemos unmap de las varaibles ya mapeadas */
        munmap(sort_comp, sizeof(*sort_comp));
        munmap(contador, sizeof(*contador));
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
    }

    hijoMask.sa_handler = manejador_SIGTERM;
    if (sigaction(SIGTERM, &hijoMask, NULL) < 0) {
        perror("sigaction");

        /* Hacemos unmap de las varaibles ya mapeadas */
        munmap(sort_comp, sizeof(*sort_comp));
        munmap(contador, sizeof(*contador));
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
    }

    hijoMask_SA.sa_handler = manejador_SIGALRM;
    if (sigaction(SIGALRM, &hijoMask_SA, NULL) < 0) {
        perror("sigaction");

        /* Hacemos unmap de las varaibles ya mapeadas */
        munmap(sort_comp, sizeof(*sort_comp));
        munmap(contador, sizeof(*contador));
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
    }

    padreMask_SI.sa_handler = manejador_SIGINT;
    if (sigaction(SIGINT, &padreMask_SI, NULL) < 0) {
        perror("sigaction");

        /* Hacemos unmap de las varaibles ya mapeadas */
        munmap(sort_comp, sizeof(*sort_comp));
        munmap(contador, sizeof(*contador));
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
    }

    if (argc > 4) {
        sort_comp->delay = 1e6 * atoi(argv[4]);
    }
    else {
        sort_comp->delay = 1e8;
    }

    /* Array donde guardar los pids de los hijos. +1 por el proceso ilustrador */
    hijos = (pid_t*)malloc((atoi(argv[3])+1)*sizeof(pid_t));
    if (hijos == NULL) {
        perror("malloc");

        /* Hacemos unmap de las varaibles ya mapeadas */
        munmap(sort_comp, sizeof(*sort_comp));
        munmap(contador, sizeof(*contador));
        
        /* Cerramos los recursos abiertos en la ejecución */
        mq_close(cola);
        close(shmid);
        close(shm_aux);
        sem_close(sem);
        sem_close(sem1);
        sem_close(sem2);

        /* Unlink de semaforos y colas (si estuvieran abiertos) */
        sem_unlink(SEM_NAME);
        sem_unlink(SEM_NAME1);
        sem_unlink(SEM_NAME2);
        shm_unlink(SHM_NAME);
        shm_unlink(VAR_AUX);
        mq_unlink(COLA_MEN);
        exit(EXIT_FAILURE);
    }

    /* El padre crea los procesos especificados al inicio */
    for (indice_tuberia = 0; indice_tuberia < atoi(argv[3]); indice_tuberia++) {
        pid = fork();
        if (pid == -1) {
            perror("fork");

            /* Liberamos memoria dinámica */
            free(hijos);
            hijos = NULL;

            /* Hacemos unmap de las varaibles ya mapeadas */
            munmap(sort_comp, sizeof(*sort_comp));
            munmap(contador, sizeof(*contador));
            
            /* Cerramos los recursos abiertos en la ejecución */
            mq_close(cola);
            close(shmid);
            close(shm_aux);
            sem_close(sem);
            sem_close(sem1);
            sem_close(sem2);

            /* Unlink de semaforos y colas (si estuvieran abiertos) */
            sem_unlink(SEM_NAME);
            sem_unlink(SEM_NAME1);
            sem_unlink(SEM_NAME2);
            shm_unlink(SHM_NAME);
            shm_unlink(VAR_AUX);
            mq_unlink(COLA_MEN);
            
            /*Terminamos el programa*/
            exit(EXIT_FAILURE);
        }

        /* Los procesos hijo salen del loop, pero establecen su señal */
        if (pid == 0){
            /*Le asignamos sus máscaras al hijo */
            if (sigprocmask(SIG_SETMASK, &(hijoMask.sa_mask), NULL) < 0) {
                perror("sigprocmask");
                
                /* Hacemos unmap de la memoria compartida */
                munmap(sort_comp, sizeof(*sort_comp));
                munmap(contador, sizeof(*contador));

                /* Cerramos los descriptores de fichero usados
                 El padre hara unlink */
                mq_close(cola);
                sem_close(sem);
                sem_close(sem1);
                sem_close(sem2);
                close(shmid);
                close(shm_aux);

                /*Terminamos el programa*/
                exit(EXIT_FAILURE);
            }

            if (sigprocmask(SIG_SETMASK, &(hijoMask_SA.sa_mask), NULL) < 0) {
                perror("sigprocmask");
                
                /* Hacemos unmap de la memoria compartida */
                munmap(sort_comp, sizeof(*sort_comp));
                munmap(contador, sizeof(*contador));

                /* Cerramos los descriptores de fichero usados
                 El padre hara unlink */
                mq_close(cola);
                sem_close(sem);
                sem_close(sem1);
                sem_close(sem2);
                close(shmid);
                close(shm_aux);
    
                /*Terminamos el programa*/
                exit(EXIT_FAILURE);
            }

            /* Cambiamos el estado del hijo a esperando */
            state = 0;

            /* Iniciamos la alarma periodica */
            alarm(1);

            break;
        }

        /* Guardamos el pid del hijo */
        hijos[indice_tuberia] = pid;
    }

    /* El hijo no necesita esta variable */
    if (pid == 0) {
        free(hijos);  
        hijos = NULL;
    }  

    /* Establecemos las mascaras del padre */
    if (pid > 0) {

        /* Establecemos la mascara para la señal SIGUSR1 en el padre */
        if (sigprocmask(SIG_SETMASK, &(padreMask.sa_mask), NULL) < 0) {
            perror("sigprocmask");

            /* Liberamos memoria dinámica */
            free(hijos);
            hijos = NULL;

            /* Hacemos unmap de las varaibles ya mapeadas */
            munmap(sort_comp, sizeof(*sort_comp));
            munmap(contador, sizeof(*contador));
            
            /* Cerramos los recursos abiertos en la ejecución */
            mq_close(cola);
            close(shmid);
            close(shm_aux);
            sem_close(sem);
            sem_close(sem1);
            sem_close(sem2);

            /* Unlink de semaforos y colas (si estuvieran abiertos) */
            sem_unlink(SEM_NAME);
            sem_unlink(SEM_NAME1);
            sem_unlink(SEM_NAME2);
            shm_unlink(SHM_NAME);
            shm_unlink(VAR_AUX);
            mq_unlink(COLA_MEN);
            
            exit(EXIT_FAILURE);
        }

        /* Establecemos la segunda mascara para la señal SIGINT */
        if (sigprocmask(SIG_SETMASK, &(padreMask_SI.sa_mask), NULL) < 0) {
            perror("sigpromask");
            
            /* Liberamos memoria dinámica */
            free(hijos);
            hijos = NULL;

            /* Hacemos unmap de las varaibles ya mapeadas */
            munmap(sort_comp, sizeof(*sort_comp));
            munmap(contador, sizeof(*contador));
            
            /* Cerramos los recursos abiertos en la ejecución */
            mq_close(cola);
            close(shmid);
            close(shm_aux);
            sem_close(sem);
            sem_close(sem1);
            sem_close(sem2);

            /* Unlink de semaforos y colas (si estuvieran abiertos) */
            sem_unlink(SEM_NAME);
            sem_unlink(SEM_NAME1);
            sem_unlink(SEM_NAME2);
            shm_unlink(SHM_NAME);
            shm_unlink(VAR_AUX);
            mq_unlink(COLA_MEN);

            exit(EXIT_FAILURE);
        }
    }

    /* Creamos el proceso ilustrador */
    if (pid > 0) {
        pid = fork();
        if (pid == -1) {
            perror("fork");

            /* Liberamos memoria dinámica */
            free(hijos);
            hijos = NULL;

            /* Hacemos unmap de las varaibles ya mapeadas */
            munmap(sort_comp, sizeof(*sort_comp));
            munmap(contador, sizeof(*contador));
            
            /* Cerramos los recursos abiertos en la ejecución */
            mq_close(cola);
            close(shmid);
            close(shm_aux);
            sem_close(sem);
            sem_close(sem1);
            sem_close(sem2);

            /* Unlink de semaforos y colas (si estuvieran abiertos) */
            sem_unlink(SEM_NAME);
            sem_unlink(SEM_NAME1);
            sem_unlink(SEM_NAME2);
            shm_unlink(SHM_NAME);
            shm_unlink(VAR_AUX);
            mq_unlink(COLA_MEN);


            /*Terminamos el programa*/
            exit(EXIT_FAILURE);
        }
        /* Ejecución del ilustrador */
        else if (pid == 0) {

            /* El ilustrador establece su manejador (la misma que los trabajadores) */
            if (sigprocmask(SIG_SETMASK, &(hijoMask.sa_mask), NULL) < 0) {
                perror("sigprocmask");

                /* Liberamos memoria dinámica */
                free(hijos);
                hijos = NULL;

                /* Hacemos unmap de las varaibles ya mapeadas */
                munmap(sort_comp, sizeof(*sort_comp));
                munmap(contador, sizeof(*contador));
                
                /* Cerramos los recursos abiertos en la ejecución */
                mq_close(cola);
                close(shmid);
                close(shm_aux);
                sem_close(sem);
                sem_close(sem1);
                sem_close(sem2);

                exit(EXIT_FAILURE);
            }

            /* Ejecucion del ilustrador */
            while(1) {

                sleep(1);

                /* Imprimimos el vector */
                plot_vector(sort_comp->data, sort_comp->n_elements);

            }
        }

        /* Guardamos el pid del proceso ilustrador */
        hijos[sort_comp->n_processes+1] = pid;
    }

    /* Bucle para que el padre mande las tareas */
    for (int i = 0; i < sort_comp->n_levels && pid > 0; i++) {

        /* Los trabajadores han terminado o no han empezado, por lo tanto podemos modificar la
         variable sin problemas */
        while (contador_p_restantes != 0) {
            sem_wait(sem1);
            contador_p_restantes = contador->contador;
            sem_post(sem1);
        }

        /* Reseteamos el contador */
        sem_wait(sem1);
        contador->contador = get_number_parts(i, sort_comp->n_levels);
        sem_post(sem1);

        contador_p_restantes = get_number_parts(i, sort_comp->n_levels);

        /* Establecemos el mensaje */
        for (int n = 0; n < get_number_parts(i, sort_comp->n_levels); n++) {
            msg[n].tarea_enviada = &sort_comp->tasks[i][n];
        }

        for (int n = 0; n < get_number_parts(i, sort_comp->n_levels); n++) {
            
            /* Metemos en la cola de mensajes todas las tareas del nivel */
            sem_wait(sem2);
            sort_comp->tasks[i][n].completed = SENT;
            sem_post(sem2);

            SEND: if (mq_send(cola, (char *) &msg[n], sizeof(msg[n]), 1) == -1) {

                /* Si durante el estado de bloqueo recibe una señal, 
                vuelve a tratar de enviar el mensaje */
                if (errno == EINTR) goto SEND;
                else { 
                    fprintf(stderr, "Error mandando el mensaje\n");
                    perror("mq_send");
                    
                    /* Liberamos memoria dinámica */
                    free(hijos);
                    hijos = NULL;

                    /* Hacemos unmap de las varaibles ya mapeadas */
                    munmap(sort_comp, sizeof(*sort_comp));
                    munmap(contador, sizeof(*contador));
                    
                    /* Cerramos los recursos abiertos en la ejecución */
                    mq_close(cola);
                    close(shmid);
                    close(shm_aux);
                    sem_close(sem);
                    sem_close(sem1);
                    sem_close(sem2);

                    /* Unlink de semaforos y colas (si estuvieran abiertos) */
                    sem_unlink(SEM_NAME);
                    sem_unlink(SEM_NAME1);
                    sem_unlink(SEM_NAME2);
                    shm_unlink(SHM_NAME);
                    shm_unlink(VAR_AUX);
                    mq_unlink(COLA_MEN);
                 
                    exit(EXIT_FAILURE);
                }
            }
        }

        /* Esperamos a que todas las tareas del nivel esten completas */
        sem_wait(sem);
        while(boolean == FALSE) {

            boolean = TRUE;

            /* Bucle donde se comprueba el estado de las tares  */
            for (int n = 0; n < get_number_parts(i, sort_comp->n_levels); n++) {
                sem_wait(sem2);
                /* Si una tarea esta incompleta el padre ejecutara el bucle while hasta que no lo este */
                if (sort_comp->tasks[i][n].completed != COMPLETED) boolean = FALSE;
                sem_post(sem2);
            }
            if (boolean == TRUE) break;
        }
        /* Reseteamos la variable para el bucle */
        boolean = FALSE;
    }

    /* El padre envia las señales a los hijos */
    if (pid > 0) {
        for (int i = 0; i < sort_comp->n_processes+2; i++) kill(hijos[i], SIGTERM);
    }

    /* Los hijos se crean su propia cola de mensajes*/
    if (pid == 0) {
        /* Cola creada para que los trabajadores solo reciban mensajes de la cola */
        queue = mq_open(COLA_MEN, O_CREAT | O_RDONLY, S_IRUSR | S_IWUSR, &attributes);
        if(queue == (mqd_t)-1) {
            fprintf(stderr, "Error opening the queue\n");
            mq_close(cola);
            sem_close(sem);
            sem_close(sem1);
            sem_close(sem2);
            munmap(sort_comp, sizeof(*sort_comp));
            close(shmid);
            
            return EXIT_FAILURE;
        }
    }

    /* Ejecucion del hijo */
    while(pid == 0) {

        mensaje msg_recibido;

        /* Los trabajadores reciben los mensajes */
        RECIBIR: if (mq_receive(queue, (char *)&msg_recibido, sizeof(msg_recibido), NULL) == -1) {
            /* Si recibe como error EINTR significa que se ha ejecutado un manejador mientras estaba bloqueado */ 

            if (errno == EINTR) goto RECIBIR;

            fprintf(stderr, "Error receiving message\n");
            
            /* Hacemos unmap de la memoria compartida */
            munmap(sort_comp, sizeof(*sort_comp));
            munmap(contador, sizeof(*contador));

            /* Cerramos los descriptores de fichero usados
                El padre hara unlink */
            mq_close(cola);
            sem_close(sem);
            sem_close(sem1);
            sem_close(sem2);
            close(shmid);
            close(shm_aux);

            exit(EXIT_FAILURE);
        }
        
        /* Cambiamos el estado de la tarea del hijo */
        CAMBIAR_ESTADO_PROC: if (sem_wait(sem2) == -1) {
            if (errno == EINTR) goto CAMBIAR_ESTADO_PROC;
        }
        msg_recibido.tarea_enviada->completed = PROCESSING;
        sem_post(sem2);

        /* El proceso cambia su propio estado para saber que esta ordenando */
        if (msg_recibido.tarea_enviada->mid == NO_MID) {
            state = 1*100000000; /* Marcamos bubblesort */
            state += msg_recibido.tarea_enviada->ini*10000; /* Marcamos primer indice */
            state += msg_recibido.tarea_enviada->end;
        } else {
            state = 1*200000000; /* Marcamos mergesort */
            state += msg_recibido.tarea_enviada->ini*10000; /* Marcamos primer indice */
            state += msg_recibido.tarea_enviada->end;
        }

        /* Se le asigna una tarea al proceso hijo (trabajador)
        y este sale del bucle */
        if (solve_task_by_task(sort_comp, msg_recibido.tarea_enviada) == ERROR) {
            
            /* Hacemos unmap de la memoria compartida */
            munmap(sort_comp, sizeof(*sort_comp));
            munmap(contador, sizeof(*contador));

            /* Cerramos los descriptores de fichero usados
                El padre hara unlink */
            mq_close(cola);
            sem_close(sem);
            sem_close(sem1);
            sem_close(sem2);
            close(shmid);
            close(shm_aux);
        
            exit(EXIT_FAILURE);
        }

        /* El proceso cambia su propio estado para saber que esta esperando a una tarea */
        state = 0;

        /* Cambiamos el estado de la tarea a completada */
        CAMBIAR_ESTADO_COMP: if (sem_wait(sem2) == -1) {
            if (errno == EINTR) goto CAMBIAR_ESTADO_COMP;
        }

        msg_recibido.tarea_enviada->completed = COMPLETED;
        sem_post(sem2);

        /* El proceso hijo accede a la memoria compartida */
        DECREMENTAR_CONTADOR: if (sem_wait(sem1) == -1) {
            if (errno == EINTR) goto DECREMENTAR_CONTADOR;
        }

        /* Comprobamos que este proceso sea el último en ejecutarse para así
        dejar al padre continuar con el siguiente nivel */
        contador->contador -= 1;
        if (contador->contador == 0) sem_post(sem);
        sem_post(sem1);

        /* El trabajador manda la señal SIGUSR1 al padre */
        kill(getppid(), SIGUSR1);
    }

    plot_vector(sort_comp->data, sort_comp->n_elements);

    /* Liberamos memoria dinámica */
    free(hijos);
    hijos = NULL;

    /* Hacemos unmap de las varaibles ya mapeadas */
    munmap(sort_comp, sizeof(*sort_comp));
    munmap(contador, sizeof(*contador));

    /* Cerramos los recursos abiertos en la ejecución */
    mq_close(cola);
    close(shmid);
    close(shm_aux);
    sem_close(sem);
    sem_close(sem1);
    sem_close(sem2);
    
    /* Unlink de semaforos y colas (si estuvieran abiertos) */
    sem_unlink(SEM_NAME);
    sem_unlink(SEM_NAME1);
    sem_unlink(SEM_NAME2);
    shm_unlink(SHM_NAME);
    shm_unlink(VAR_AUX);
    mq_unlink(COLA_MEN);

    return EXIT_SUCCESS;
}

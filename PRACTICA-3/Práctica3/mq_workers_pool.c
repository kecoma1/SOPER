/**
 * @file mq_workers_pool.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Este programa esta relacionado con el programa mq_injector,
 * el anterior se encarga de mandar mensajes y este se encarga de crear N
 * trabajadores los cuales reciben estos mensajes y buscan un caracter en 
 * concreto
 * @version 1
 * @date 2020-04-11
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_SIZE 2

/* Contadores para los procesos. Son variables globales para que en los manejadores se pueda imprimir
   la estadística que nos piden, y como los procesos cada uno tienen su propio BCP cada proceso tendra  
   su propio contador */
int mensajes_leidos = 0;
int caracteres_encontrados = 0;
pid_t *pid = NULL;
char *nombre_cola;
int N = 0;
mqd_t cola;

/**
 * @brief Estructura que define un mensaje
 * 
 */
typedef struct {
    char contenido[MAX_SIZE + 1];
} Mensaje;

/**
 * @brief Funcion para los procesos que reciban la señal SIGTERM
 * 
 * @param sig 
 */
void manejador_SIGTERM(int sig) {
    double d;
    d = (double)caracteres_encontrados/(double)mensajes_leidos;

    printf("Proceso: %d\tmensajes procesados: %d\tCasos de exito: %d\tEfectividad %.2lf%c\n", getpid(), mensajes_leidos, caracteres_encontrados, d*100, '%');
    mq_close(cola);
    free(pid);
    free(nombre_cola);
    exit(EXIT_SUCCESS);
}

/**
 * @brief Manejador para los procesos que reciban la señal SIGUSR2
 * 
 * @param sig Señal
 */
void manejador_SIGUSR2(int sig) {
    for(int i = 0; i < N; i++)kill(pid[i], SIGTERM);
    while(wait(NULL) != -1);
    free(pid);
    free(nombre_cola);
    mq_close(cola);
    exit(EXIT_SUCCESS);
}

int main(int argc, char **argv) {
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_curmsgs = 0,
        .mq_msgsize = sizeof(Mensaje)
    };

    struct sigaction mask_gerente;
    struct sigaction mask_worker;
    char c;
    int fin = 0;
    char contenido[MAX_SIZE+1];
    pid_t pid_aux;

    /* Comprobando argumentos */
    if (argc < 4 || argc > 4) {
        fprintf(stderr, "Argumentos invalidos, recuerde: ./workers_pool <trabajadores> <nombre de la cola mensajes> <caracter>\n");
        exit(EXIT_FAILURE);
    }

    N = atoi(argv[1]);
    if (N < 1 || N > 10) {
        fprintf(stderr, "N debe estar entre 1 y 10\n");
        exit(EXIT_FAILURE);
    }

    c = argv[3][0];

    /* Reservamos memoria para el nombre de la cola */
    nombre_cola = (char *)malloc((strlen(argv[2])+1)*sizeof(char));
    if (nombre_cola == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    strcpy(nombre_cola, argv[2]);

    /* Creando mascaras */
    sigfillset(&(mask_gerente.sa_mask));
    mask_gerente.sa_flags = 0;

    /* Eliminamos la única señal que esperamos */
    sigdelset(&(mask_gerente.sa_mask), SIGUSR2);

    if (sigprocmask(SIG_SETMASK, &(mask_gerente.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    /* Establecemos el manejador */
    mask_gerente.sa_handler = manejador_SIGUSR2;
    if (sigaction(SIGUSR2, &mask_gerente, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Reservamos memoria para los guardar los pid */
    pid = (pid_t*)malloc(N*sizeof(pid_t));
    if (pid == NULL) {
        free(nombre_cola);  
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    pid_aux = 1;

    /* Creamos los hijos */
    for (int i = 0; i < N && pid_aux != 0; i++) {
        pid_aux = fork();
        if (pid_aux == -1) {
            perror("fork");
            free(nombre_cola);
            free(pid);
            exit(EXIT_FAILURE);
        }
        pid[i] = pid_aux;
    }


    if (pid_aux == 0) {
        /* Abriendo la cola */
        cola = mq_open(argv[2], O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, &attributes);
        
        if (cola == (mqd_t)-1) {
            fprintf(stderr, "Error abriendo la cola de mensajes\n");
            exit(EXIT_FAILURE);
        }

        /* Creamos la mascara para los trabajadores */
        sigfillset(&(mask_worker.sa_mask));
        mask_worker.sa_flags = 0;

        /* Eliminamos la única señal que esperamos */
        sigdelset(&(mask_worker.sa_mask), SIGTERM);

        if (sigprocmask(SIG_SETMASK, &(mask_worker.sa_mask), NULL) < 0) {
            perror("sigprocmask");
            exit(EXIT_FAILURE);
        }

        /* Establecemos el manejador */
        mask_worker.sa_handler = manejador_SIGTERM;
        if (sigaction(SIGTERM, &mask_worker, NULL) < 0) {
            perror("sigaction");
            exit(EXIT_FAILURE);
        }

        Mensaje msg;
        mensajes_leidos = 0;
        caracteres_encontrados = 0;

        /* Bucle para leer mensaje */
        while(1) {
            mq_receive(cola, (char *)&msg, sizeof(msg), NULL);

            strcpy(contenido, msg.contenido);

            /* Comprobamos si es el ultimo mensaje
               El primer y ultimo caracter son iguales a \0
               si es el últmo caracter */
            if (contenido[0] == '\0' &&
                contenido[MAX_SIZE+1] == '\0') {
                    kill(getppid(), SIGUSR2);
                    sigsuspend(&(mask_worker.sa_mask));
                }

            /* Buscamos el caracter */
            for (int i = 0; i < MAX_SIZE; i++) {
                if (c == contenido[i]) caracteres_encontrados++;
            }

            mensajes_leidos++;
        } 

    }
    /* Suspendemos al gerente (padre) hasta que reciba la señal sigusr2*/ 
    else sigsuspend(&(mask_gerente.sa_mask));

    return 1;
}
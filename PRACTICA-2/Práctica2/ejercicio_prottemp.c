/**
 * @file ejercicio_prottemp.c
 * @author 
 *      Kevin de la Coba Malam (kevin.coba@estudiante.uam.es)
 *      Jose Manuel Freire (jose.freire@estudiante.uam.es)
 * @brief Programa que comunica procesos entre sí mediante el uso de señales
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int N, T;       //Parametros introducidos
int usr2 = 0;   //Contador para las señales SIGUSR2
pid_t* list;    //Puntero que se convertira en un array de PID

/**
 * @brief Manejador de la señal SIGALRM
 *  Envia a todos los hijos la señal SIGTERM, comprueba que los hijos terminen de ejecutarse y despues
 *  finaliza
 * 
 * @param sig Señal
 */
void manejador_SIGALRM(int sig) {
    
    /* Enviando señal SIGTERM a todos los hijos */
    for (int i = 0; i < N; i++) kill(list[i], SIGTERM);

    /* Esperando a todos los hijos */
    while(wait(NULL)!=-1);
    printf("Finalizado Padre\n");
    free(list);
    exit(0);
}

/**
 * @brief Manejador para la señal SIGTERM
 * Principalmente va a ser usada por los hijos, en cuanto reciban la señal
 * terminan su proceso
 * 
 * @param sig Señal
 */
void manejador_SIGTERM(int sig) {
    printf("Finalizado %d\n", getpid());
    exit(0);
}

/**
 * @brief Manejador para la señal SIGUSR2
 * Para contar las veces que se recibe esta señal
 * 
 * @param sig Señal
 */
void manejador_SIGUSR2(int sig) {
    usr2++;
    printf("Llamadas a SIGUSR2: %d\n", usr2);
}


int main(int argc, char **argv){

    struct sigaction padreMask;
    struct sigaction hijoMask;
    struct sigaction usr2Mask;
    int pid = 1, padre = 0, resultado = 0;

    /* Comprobando parametros */
    if (argc < 3) {
        printf("%s <N>(numero de procesos) <T>(tiempo de espera)\n", argv[0]);
        return(EXIT_FAILURE);
    }

    /* Traduciendo argumentos */
    N = atoi(argv[1]);
    T = atoi(argv[2]);

    /* Comprobando los argumentos introducidos */
    if(N < 1) { 
        printf("Invalid N\n");
        return 1;
    }
    if(T < 0) {
        printf("Invalid T\n");
        return 1;
    }

    /* Creando mascaras */
    sigfillset(&(padreMask.sa_mask));
    padreMask.sa_flags = 0;

    /* El proceso padre espera la señal SIGALRM y SIGUSR2,
     las eliminamos de la mascara */
    sigdelset(&(padreMask.sa_mask), SIGALRM);
    sigdelset(&(padreMask.sa_mask), SIGUSR2);

    if (sigprocmask(SIG_SETMASK, &(padreMask.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    sigfillset(&(hijoMask.sa_mask));
    hijoMask.sa_flags = 0;

    /* El hijo espera SIGTERM */
    sigdelset(&(hijoMask.sa_mask), SIGTERM);

    if (sigprocmask(SIG_SETMASK, &(hijoMask.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }    

    sigfillset(&(usr2Mask.sa_mask));
    usr2Mask.sa_flags = 0;

    /* Espera la señal SIGUSR2 */
    sigdelset(&(usr2Mask.sa_mask), SIGUSR2);

    if (sigprocmask(SIG_SETMASK, &(usr2Mask.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    /* Estableciendo manejadores */
    padreMask.sa_handler = manejador_SIGALRM;
    if (sigaction(SIGALRM, &padreMask, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    hijoMask.sa_handler = manejador_SIGTERM;
    if (sigaction(SIGTERM, &hijoMask, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    usr2Mask.sa_handler = manejador_SIGUSR2;
    if (sigaction(SIGUSR2, &usr2Mask, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    /* Allocamos memoria para la los pid de los hijos */
    list = (pid_t*)malloc(N*sizeof(pid_t));
    if (list == NULL) {
        perror("Malloc");
        exit(EXIT_FAILURE);
    }

    /* Guardamos el pid del padre */
    padre = getpid();

    /* Creamos los hijos */
    for (int i = 0; i < N && pid > 0; i++){
        pid = fork();
        if (pid > 0) list[i] = pid;
    }

    /* El padre hace esto */
    if (padre == getpid()) {
        if (alarm(T)) {
            printf("Existe una alarma previa establecida\n");
        }
    } 
    /* El hijo hace esto */ 
    else {
        for(int i = 1; i < getpid()/10; i++) resultado += i;
        printf("PID: %d\t%d\n", getpid(), resultado);
        kill(padre, SIGUSR2);
        free(list);
        sigsuspend(&(hijoMask.sa_mask));
    }

    while (1) sigsuspend(&(padreMask.sa_mask));
}
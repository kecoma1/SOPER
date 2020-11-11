/**
 * @file ejercicio_prottemp_mejorado.c
 * @author 
 *      Kevin de la Coba Malam (kevin.coba@estudiante.uam.es)
 *      Jose Manuel Freire (jose.freire@estudiante.uam.es)
 * @brief Programa que comunica procesos entre sí mediante el uso de señales y semaforos
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
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#define SEM_NAME "/example_sem" //Nombre del semaforo

int N, T;                   //Parametros introducidos
int usr2 = 0;               //Contador para las señales SIGUSR2
int procesos_escritos = 0;  //Variable para contar el numero de procesos que han interactuado con el FILE
pid_t* list;                //Puntero que se convertira en un array de PID
sem_t *sem = NULL;          //Semaforo para controlar el uso del FILE
FILE *pf = NULL;            //File donde se va a guardar la información

/**
 * @brief Manejador de la señal SIGALRM
 *  Envia a todos los hijos la señal SIGTERM, comprueba que los hijos hayan acabado y despues
 *  finaliza
 * 
 * @param sig Señal
 */
void manejador_SIGALRM(int sig) {
    int valor = 0;

    /* Enviando señal SIGTERM a todos los hijos */
    for (int i = 0; i < N; i++) kill(list[i], SIGTERM);

    /* Esperando a todos los hijos */
    while(wait(NULL)!=-1);

    /* Leyendo datos */
    pf = fopen("data.txt", "r");
    if (pf == NULL) {
        perror("fopen");
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }
    fscanf(pf, "%d\n%d", &procesos_escritos, &valor);
    fclose(pf);

    while(wait(NULL)!=-1);
    if (procesos_escritos < N) printf("Falta trabajo\n");
    else printf("Han acabado todos, resultado: Nº procesos %d, valor: %d\n", procesos_escritos, valor);
    free(list);
    sem_close(sem);
    sem_unlink(SEM_NAME);
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

    /* Leemos el archivo */
    sem_wait(sem);
    pf = fopen("data.txt", "r");
    if (pf == NULL) {
        perror("fopen");
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }
    fscanf(pf, "%d\n", &procesos_escritos);
    fclose(pf);
    sem_post(sem);

    if (procesos_escritos == N) {
        /* Enviando señal SIGTERM a todos los hijos */
        for (int i = 0; i < N; i++) kill(list[i], SIGTERM);
    }
    
}

int main(int argc, char **argv){

    struct sigaction padreMask;
    struct sigaction hijoMask;
    struct sigaction usr2Mask;
    int pid = 1, padre = 0, resultado = 0;
    int ln1 = 0, ln2 = 0;

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

    /* Creando semaforo */
    if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
		perror("sem_open");
        sem_unlink(SEM_NAME);
		exit(EXIT_FAILURE);
	}

    /* Abriendo archivo */
    pf = fopen("data.txt", "w+");
    if (pf == NULL) {
        perror("fopen");
        sem_unlink(SEM_NAME);
        return(EXIT_FAILURE);
    }
    fprintf(pf, "0\n0");
    fclose(pf);

    /* Creando mascaras */
    sigfillset(&(padreMask.sa_mask));
    padreMask.sa_flags = 0;

    /* El proceso padre espera la señal SIGALRM y SIGUSR2,
     las eliminamos de la mascara */
    sigdelset(&(padreMask.sa_mask), SIGALRM);
    sigdelset(&(padreMask.sa_mask), SIGUSR2);

    if (sigprocmask(SIG_SETMASK, &(padreMask.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        sem_unlink(SEM_NAME);
        return(EXIT_FAILURE);
    }

    sigfillset(&(hijoMask.sa_mask));
    hijoMask.sa_flags = 0;

    /* El hijo espera SIGTERM */
    sigdelset(&(hijoMask.sa_mask), SIGTERM);

    if (sigprocmask(SIG_SETMASK, &(hijoMask.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        sem_unlink(SEM_NAME);
        return(EXIT_FAILURE);
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
        sem_unlink(SEM_NAME);
        return(EXIT_FAILURE);
    }

    hijoMask.sa_handler = manejador_SIGTERM;
    if (sigaction(SIGTERM, &hijoMask, NULL) < 0) {
        perror("sigaction");
        sem_unlink(SEM_NAME);
        return(EXIT_FAILURE);
    }

    usr2Mask.sa_handler = manejador_SIGUSR2;
    if (sigaction(SIGUSR2, &usr2Mask, NULL) < 0) {
        perror("sigaction");
        sem_unlink(SEM_NAME);
        return(EXIT_FAILURE);
    }

    /* Allocamos memoria para la los pid de los hijos */
    list = (pid_t*)malloc(N*sizeof(pid_t));
    if (list == NULL) {
        perror("Malloc");
        sem_unlink(SEM_NAME);
        return(EXIT_FAILURE);
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

        /* Bajamos el valor del semaforo para que el sea el único que acceda en ese momento */
        sem_wait(sem);

        /* Leyendo y escribiendo en el archivo */
        pf = fopen("data.txt", "r");
        if (pf == NULL) {
            perror("fopen");
            sem_unlink(SEM_NAME);
            free(list);
            return(EXIT_FAILURE);
        }
        fscanf(pf, "%d\n%d", &ln1, &ln2);
        fclose(pf);

        pf = fopen("data.txt", "w+");
        if (pf == NULL) {
            perror("fopen");
            sem_unlink(SEM_NAME);
            free(list);
            return(EXIT_FAILURE);
        }
        fprintf(pf, "%d\n%d", ln1+1, ln2+resultado);
        fclose(pf);

        /* Aumentamos el valor del semaforo y dejamos a otro pasar */
        sem_post(sem);

        kill(padre, SIGUSR2);
        free(list);
        sem_close(sem);
        sem_unlink(SEM_NAME);
        sigsuspend(&(hijoMask.sa_mask));
    }

    while (1) sigsuspend(&(padreMask.sa_mask));
}
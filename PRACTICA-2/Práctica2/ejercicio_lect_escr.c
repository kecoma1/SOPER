/**
 * @file ejercicio_lect_escr.c
 * @author Kevin de la Coba (kevin.coba@estudiante.uam.es)
 * @author Jose Manuel Freire (jose.freire@estudiante.uam.es)
 * @brief Un programa que coordina la lectura y escritura de un proceso paddre y sus hijos.
 * @version 1.0
 * @date 2020-03-16
 * 
 * - Grupo: 2291
 * - Pareja: 5
 * 
 * @copyright Copyright (c) 2020
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define NAMEA "/semaforoLectura"
#define NAMEB "/semaforoEsritura"

#define N_READ 10
#define SECS 0

//Declaracion variables globales
sem_t *semaforoLectura, *semaforoEsritura;
pid_t temp = 1;
int lectores=0;

pid_t *lista = NULL;

/**
 * @brief Manejador para la señal Sigint
 * 
 * @param sig Señal
 */
void manejadorSigint(int sig){
    int i;
    if(temp!=0){
        /* Mandando sigterm a todos los hijos */
       for(i=0; i<N_READ; i++){
    		kill(lista[i], 15);
            wait(NULL);
    	}

        printf("\nHe terminado!\n");
        free(lista);
        sem_close(semaforoLectura);
        sem_unlink(NAMEA);
        sem_close(semaforoEsritura);
        sem_unlink(NAMEB);

        exit(EXIT_SUCCESS);
    }
}

/**
 * @brief Manejador Sigterm
 * 
 * @param sig Señal
 */
void manejadorSigterm(int sig){

    free(lista);
    sem_close(semaforoLectura);
    sem_close(semaforoEsritura);

    if(temp==0){
        exit(EXIT_SUCCESS);
    }

}

/**
 * @brief Funcion que simula la escritura
 * 
 */
void escribir(){
    printf("W-INI <%d>\n", getpid());
    sleep(1);
    printf("W-FIN <%d>\n", getpid());
    fflush(stdout);
}

/**
 * @brief Funcion que maneja la sincronizacion de la escritura
 * 
 * @param semaforoEsritura Semaforo para la escritura
 * @return int 
 */
int escritura(sem_t *semaforoEsritura){
    if(semaforoEsritura==NULL) return -1;
    sem_wait(semaforoEsritura);
    escribir();
    sem_post(semaforoEsritura);

    return 0;
}

/**
 * @brief Función que maneja leer
 * 
 */
void leer(){
    printf("R-INI <%d>\n", getpid());
    sleep(1);
    printf("R-FIN <%d>\n", getpid());
    fflush(stdout);
}

/**
 * @brief Funcion para la sincronizacion de la lectura
 * 
 * @param semaforoLectura Semaforo para la lectura
 * @param semaforoEsritura Semaforo para la escritura
 * @return int Flag
 */
int lectura(sem_t *semaforoLectura, sem_t *semaforoEsritura){

    if(semaforoEsritura==NULL || semaforoLectura==NULL) return -1;
    
    sem_wait(semaforoLectura);
    
    lectores++;
    if (lectores == 1){
        sem_wait(semaforoEsritura);
    }
    sem_post(semaforoLectura);

    leer();
    
    sem_wait(semaforoLectura);
    lectores--;
    if(lectores == 0){
        sem_post(semaforoEsritura);
    }
    sem_post(semaforoLectura);

    return 0;
}

int main(){
	struct sigaction act_INT, act_TERM;
    int i;

    /* Estableciendo la primera mascara para sigint */
	sigemptyset(&(act_INT.sa_mask));
	act_INT.sa_flags = 0;

    /* Estableciendo el manejador para sigint */
	act_INT.sa_handler = manejadorSigint;
    
	if (sigaction(SIGINT, &act_INT, NULL) < 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

    /* Estableciendo la segunda mascara */
    sigemptyset(&(act_TERM.sa_mask));
	act_TERM.sa_flags = 0;

    /* Estableciendo el manejador para sigterm */
	act_TERM.sa_handler = manejadorSigterm;
	if (sigaction(SIGTERM, &act_TERM, NULL) < 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

    /* Inicializando el semaforo de lectura */
    if ((semaforoLectura = sem_open(NAMEA, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
		perror("sem_open");
		exit(EXIT_FAILURE);
	}
    
    /* Inicializando el semaforo de escritura */
    if ((semaforoEsritura = sem_open(NAMEB, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 1)) == SEM_FAILED) {
		perror("sem_open");
        sem_close(semaforoLectura);
        sem_unlink(NAMEA);
		exit(EXIT_FAILURE);
	}

    /* Inicializando la lista de PID para los procesos hijos */
	lista=(pid_t *)calloc(N_READ, sizeof(pid_t));
	if(lista==NULL){
        
		sem_close(semaforoLectura);
		sem_unlink(NAMEA);
        sem_close(semaforoEsritura);
        sem_unlink(NAMEB);

		exit(EXIT_FAILURE);
	}

    printf("pid: %d\n", getpid());

    /* Creando hijos */
    for(i=0;i<N_READ && temp>0;i++){
        if((temp = fork()) < 0){
            sem_close(semaforoLectura);
            sem_close(semaforoEsritura);
            sem_unlink(NAMEA);
            sem_unlink(NAMEB);
            exit(EXIT_FAILURE);
        }
        if(temp>0){
            lista[i] = temp;
        } else if(temp==0){
            /* El hijo ejecutara su lectura */
            while(lectura(semaforoLectura, semaforoEsritura)>=0){
                sleep(SECS);
            }
            exit(EXIT_FAILURE);
        }
    }

    /* El padre ejecutara su escritura */
    while(escritura(semaforoEsritura)>=0){
        sleep(SECS);
    }
    
    exit(EXIT_FAILURE);
}
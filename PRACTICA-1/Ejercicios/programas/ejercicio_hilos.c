#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

typedef struct{
    int random;
    int x;
} Args;

/**
 * @brief 
 * 
 * @param arg 
 * @return void* 
 */
void *xcube(void *arg) {

    int *resultado = NULL;
    Args *a = NULL;
    a = (Args*) arg;

    resultado = (int*)malloc(sizeof(int));
    if (resultado == NULL) {

        perror("xcube");
        return NULL;
    }
    sleep(a->random);

    *resultado = (int)((a->x)*(a->x)*(a->x));    
    return (void*)resultado;
}

int main(int argc, char **argv){
   
    pthread_t *threads = NULL;
    Args **args = NULL;
    int **resultados = NULL, flag = 0;

    srand(time(NULL));

    if(argc<2){
        printf("Usar: ./a.out <número de hilos>\n");
        return 1;
    }

    /* Reservando memoria para los threads y otras variables */
    threads = (pthread_t*)malloc(atoi(argv[1])*sizeof(pthread_t));
    if (threads == NULL) {
        perror("malloc");
        return(EXIT_FAILURE);
    }

    args = (Args**)malloc(atoi(argv[1])*sizeof(Args*));
    if (args == NULL) {

        free(threads);
        perror("malloc");
        return(EXIT_FAILURE);
    }

    for (int i = 0; i < atoi(argv[1]); i++) {
        args[i] = (Args*)malloc(atoi(argv[1])*sizeof(Args));
        if (args == NULL) {
            free(threads);
            for(int n = 0; n < i ; n++) free(args[n]);
            free(args);
            perror("malloc");
            return(EXIT_FAILURE);
        }
    }

    resultados = (int**)malloc(atoi(argv[1])*sizeof(int*));
    if (resultados == NULL) {

        free(threads);
        for(int i = 0; i < atoi(argv[1]); i++) free(args[i]);
        free(args);
        perror("malloc");
        return(EXIT_FAILURE);
    }

    /* Creando los hilos */
    for (int i = 0; i < atoi(argv[1]); i++) {
        args[i]->random = rand()%11;
        args[i]->x = i+1;
        flag = pthread_create(&threads[i], NULL, xcube, args[i]);
        if(flag!=0){
            free(threads);
            for(int n = 0; n < atoi(argv[1]); n++) free(args[i]);
            free(args);
            for(int n = 0; n < i; n++) free(resultados[i]);
            free(resultados);
            perror("pthread_create");
            return(EXIT_FAILURE);
        }
    }

    /* Esperando los hilos */
    for (int i = 0; i < atoi(argv[1]); i++) {
        pthread_join(threads[i], (void*)&resultados[i]);
        if(flag!=0){
            free(threads);
            for(int n = 0; n < atoi(argv[1]); n++) free(args[i]);
            free(args);
            for(int n = 0; n < i; n++) free(resultados[i]);
            free(resultados);
            perror("pthread_create");
            return(EXIT_FAILURE);
        }
    }

    for(int i = 0; i < atoi(argv[1]); i++) printf("Hilo %d, return = %d\n", i+1, *resultados[i]);

    /* Liberando memoria */
    for(int i = 0; i < atoi(argv[1]); i++) {
        free(args[i]);
        free(resultados[i]);
    }
    free(resultados);
    free(args);
    free(threads);

}
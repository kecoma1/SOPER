#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>


#define MAX_THREADS 100

typedef struct{
    int random;
    int x;
}Args;

void * xcube(void *voidp){
    int *resultado = NULL;
    
    resultado = (int*)malloc(sizeof(int));
    if (resultado == NULL) {

        perror("xcube");
        return NULL;
    }
    Args* args;
    args = (Args*) voidp;
    sleep(args->random);
    
    return (void*)resultado;
}


int main(int argc, char **argv){
    int hilos, error, i;
    pthread_t* threads=NULL;
    Args **args=NULL;
    int *voidp;
    
    
	if(argc<2){
        printf("Usar: ./a.out <número de hilos>\n");
        return 1;
    }
    
    hilos = atoi(argv[1]);
    if(hilos>MAX_THREADS) hilos = MAX_THREADS;
    
    threads = (pthread_t*) malloc (hilos*sizeof(pthread_t));
    if(!threads){
        perror("malloc");
        printf("Ha habido un error al alojar la memoria de threads.\n");
        return(EXIT_FAILURE);
    }
    args = (Args**) malloc (hilos*sizeof(Args*));
    if(!args){
        perror("malloc");
        printf("Ha habido un error al alojar la memoria args.\n");
        return(EXIT_FAILURE);
    }
    
    for(i=0;i<hilos;i++){
        args[i] = (Args*) malloc (sizeof(Args));
        args[i]->random = rand()%11;
        args[i]->x = i;
        error = pthread_create((threads[i]), NULL, xcube, args[i]);
        if(error!=0){
            perror("pthread_create");
            printf("Ha habido un error al crear el thread.\n");
            return 1;
        }
        error = pthread_join(threads[i], (void**)voidp);
        if(error!=0){
            perror("pthread_join");
            printf("Ha habido un error al hacer join en el thread.\n");
            return 1;
        }
        printf("El resultado es: %d\n", *voidp);
    }
    
	return 0;
}

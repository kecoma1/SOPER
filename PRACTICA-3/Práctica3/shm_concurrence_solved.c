/**
 * @file shm_concurrence_solved.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Archivo donde se soluciona el problema planteado en la practica, en 
 * concreto en el ejercicio 3
 * @version 1
 * @date 2020-04-14
 * 
 * @copyright Copyright (c) 2020
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
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>

#define SHM_NAME "/shm_eje3"
#define MAX_MSG 2000

/**
 * @brief Obtiene la hora del dia
 * 
 * @param buf String donde copiar la hora del dia
 */
static void getMilClock(char *buf) {
    int millisec;
	char aux[100];
    struct tm* tm_info;
    struct timeval tv;

    gettimeofday(&tv, NULL);
	millisec = lrint(tv.tv_usec/1000.0); // Round to nearest millisec
    if (millisec>=1000) { // Allow for rounding up to nearest second
        millisec -=1000;
        tv.tv_sec++;
    }
    tm_info = localtime(&tv.tv_sec);
    strftime(aux, 10, "%H:%M:%S", tm_info);
	sprintf(buf, "%s.%03d", aux, millisec);
}

typedef struct {
	pid_t processid;       /* Logger process PID */
	long logid;            /* Id of current log line */
	char logtext[MAX_MSG]; /* Log text */
	sem_t sem;			   /* Semaphore to control the access to the structure */
} ClientLog;

ClientLog *cl;

/**
 * @brief Manejador para la señal SIGUSR1
 * 
 * @param sig Señal
 */
void manejador (int sig) {
	if (sig == SIGUSR1) {
		printf ("Log %ld: Pid %d: %s\n",cl->logid, cl->processid, cl->logtext);
	}
}

/**
 * @brief Esta función nos permite dormir un proceso millisegundos
 * 
 * @param msec Milisegundos que duerme
 * @return int flag
 */
int millisleep(long msec){
    struct timespec ts;
    int res;

    if (msec < 0){
        errno = EINVAL;
        return -1;
    }

    ts.tv_sec = msec / 1000;
    ts.tv_nsec = (msec % 1000) * 1000000;

    do{
        res = nanosleep(&ts, &ts);
    }
	while (res && errno == EINTR);

    return res;
}

int main(int argc, char *argv[]) {
	int n, m, tot;
	int ppid = 0;
	int ret = EXIT_FAILURE;
	double timems;
	sigset_t set;
	struct sigaction sigact;

	srand(time(NULL));

	if (argc < 3) { //Comprueba el numero de argumentos
		fprintf(stderr,"usage: %s <n_process> <n_logs> \n",argv[0]);
		return ret;
	}

	n = atoi(argv[1]);
	m = atoi(argv[2]);
	tot = m*n;

	/* Crea la memoria compartida */
	int shm = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (shm == -1) {
		perror("shm_open did not created the shared memory correctly.");
        return EXIT_FAILURE;
    }

	/* Cambia el espacio de memoria compartida al de la estructura client log */
	if (ftruncate(shm, sizeof(ClientLog)) == -1) {
		perror("ftruncate did not perform successfully.");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
	}

	/* Mapea la memoria compartida a una variable del programa */
	cl = mmap(NULL, sizeof(*cl), PROT_READ | PROT_WRITE, MAP_SHARED, shm, 0);
	if(!cl){
		perror("mmap: ");
		return ret;
	}
    cl->logid = -1;
	close(shm);

	/* Comprobamos si el mapeado a fallado */
	if (cl == MAP_FAILED) {
        fprintf(stderr, "Error mapping the shared memory segment\n");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
    }

	/* Inicializa el semaforo */
	if(sem_init(&(cl->sem), 1, 1)==-1){
        perror("sem_init failed.");
		return ret;
    }

	/* Esta parte crea la mascara para las señales, haciendo que este proceso solo tenga en cuenta SIGUSR1 */
	sigemptyset(&(sigact.sa_mask));
	sigaddset(&(sigact.sa_mask), SIGUSR1);
	sigact.sa_flags = 0;
	sigact.sa_handler =  manejador;
	if(sigaction(SIGUSR1, &sigact, NULL) < 0){
		perror("Sigaction failed.");
		exit(EXIT_FAILURE);
	}
	
	/* Aqui creamos los procesos hijo */
	for(int i=0; i < n; i++){
		
        ppid=fork();
        if(ppid<0){
            perror("Fork failed.");
            exit(EXIT_FAILURE);
        }
        if(!ppid){ break; }
		
    }

	/*En esta parte damos funcionalidad al proceso padre e hijo*/
	if(ppid==0){
		//Esta parte define el comportamiento del hijo (ahora controlado por un semaforo).
		for(int i = 0; i < m; i++){
			timems = ((rand()%800) + 100);
			millisleep(timems);

			sem_wait(&(cl->sem));

			cl->logid++;

			cl->processid = getpid();
			getMilClock(cl->logtext);

			kill(getppid(), SIGUSR1);
			sem_post(&(cl->sem));
		}

	} else {
		//Esta parte define el comportamiento de el padre
		sigfillset(&set);
		sigdelset(&set, SIGUSR1);
		sigdelset(&set, SIGTSTP);

		while(cl->logid < (tot)-1){
			sigsuspend(&set);
		}

		if(sem_destroy(&(cl->sem))==-1){
			perror("sem_destroy failed.");
			return ret;
		}

		munmap(cl, sizeof(*cl));
		shm_unlink(SHM_NAME);

	}

	return ret;
}

#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define SEM_NAME "/example_sem"

void manejador(int sig) {
    return;
}

int main(void) {
	sem_t *sem = NULL;
    struct sigaction act;

	if ((sem = sem_open(SEM_NAME, O_CREAT | O_EXCL, S_IRUSR | S_IWUSR, 0)) == SEM_FAILED) {
		perror("sem_open");
        sem_unlink(SEM_NAME);
		exit(EXIT_FAILURE);
	}

    sigfillset(&(act.sa_mask));
    act.sa_flags = 0;

    if (sigprocmask(SIG_SETMASK, &(act.sa_mask), NULL) < 0) {
        perror("sigprocmask");
        exit(EXIT_FAILURE);
    }

    /* Se arma la señal SIGINT. */
    act.sa_handler = SIG_IGN;
    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

	printf("Entrando en espera (PID=%d)\n", getpid());
	sem_wait(sem);
    printf("Fin de la espera\n");
	sem_unlink(SEM_NAME);
}

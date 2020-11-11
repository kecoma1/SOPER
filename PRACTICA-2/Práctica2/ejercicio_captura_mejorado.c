#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

static volatile sig_atomic_t got_signal = 0;

/* manejador: rutina de tratamiento de la señal SIGINT. */
void manejador(int sig) {
    printf("Señal recibida %d.\n", got_signal);
    got_signal = 1;
}

int main(void) {
    struct sigaction act;

    act.sa_handler = manejador;
    sigemptyset(&(act.sa_mask));
    act.sa_flags = 0;

    if (sigaction(SIGINT, &act, NULL) < 0) {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }

    while(1) {
        printf("En espera de Ctrl+C (PID = %d)\n", getpid());
		if(got_signal) {
		  got_signal = 0;
		  printf("Señal recibida %d.\n", got_signal);
		}
        sleep(9999);
    }
}

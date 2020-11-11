#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_PROC 3

int main(void) {

    /* Iniciamos la variable a cero para poder hacer un fork */
    pid_t pid = 0, *waitVar; 

    for (int i = 0; i < NUM_PROC - 1; i++) {
        
        /* Si eres el hijo, haces un fork */
        if(pid == 0) {
            pid = fork();
        }
        else if (pid < 0) {
            perror("fork");
			exit(EXIT_FAILURE);
        } else {
            /* El padre espera por todos sus hijos */
            while(wait(NULL) != -1);
        }
    }

    return(EXIT_SUCCESS);
}

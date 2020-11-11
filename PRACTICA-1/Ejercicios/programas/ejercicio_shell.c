#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wordexp.h>
#include <errno.h>

#define MAX 100
#define MAX_ARG 100

int main() {

    char string[MAX];
    char *arg = NULL;
    char *args[MAX_ARG];
    int i = 0, n = 0, err_v = 0, flag;
    pid_t pid;

    printf(">");
    
    /* Cogiendo el comando */
    while (fgets(string, MAX, stdin) != NULL) {

        /* Separando los argumentos */
        arg = strtok(string, " ");
        args[0] = (char*)malloc(strlen(arg)*sizeof(char));
        if (args[0] == NULL) return -1;
        strcpy(args[0], arg);
        i = 1;
        while (arg != NULL) {
            
            arg = strtok(NULL, " ");
            if (arg == NULL) {
                
                args[i] = NULL;
                break;
            }
            args[i] = (char*)malloc(strlen(arg)*sizeof(char));
            if (args[i] == NULL) return -1;
            strcpy(args[i], arg);
            i++;
        }

        if (i == 1) {

            for (n = 0; args[0][n] != '\n'; n++);
            args[0][n] = '\0';
        }

        /* Creando proceso */
        pid = fork();

        if (pid == 0) {
            execv(args[0], (char* const*)args);
        } else {
            wait(&flag);
        }

        if (WIFSIGNALED(flag)) {

            fprintf(stderr, "Terminated by signal %d\n", WTERMSIG(flag));
        } else if (WIFEXITED(flag)) {
            perror("\nResult");
            fprintf(stderr, "Exited with value %d\n", flag);
        }

        printf(">");

        /* Liberando memoria */
        for(int n = 0; n < i; n++) {
            free(args[n]);
            args[n] = NULL;
        }
    }

    return -1;
}
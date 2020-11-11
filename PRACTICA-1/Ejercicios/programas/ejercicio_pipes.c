#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int main(int argc, char **argv) {

    pid_t pid;
    int tuberia1[2], tuberia2[2];
    int numero;
    int pipe_status;

    srand(time(NULL));

    FILE *pf = NULL;
    pf = fopen("numero_leido.txt", "w");
    if (pf == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }

    /* Creando tuberias */
    pipe_status = pipe(tuberia1);
	if(pipe_status == -1)
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}

    /* Creamos dos procesos hijos */
    pid = fork();
    if (pid < 0){
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {

        /* Cierre del descriptor de entrada en el hijo */
		close(tuberia1[0]);

        /* Generando número aleatorio */
        int random = rand()%11;

        ssize_t nbytes = write(tuberia1[1], &random, sizeof(int));
		if(nbytes == -1)
		{
			perror("write");
			exit(EXIT_FAILURE);
		}

        /* Imprimiendo en pantalla el numero generado */
        printf("%d", random);

        exit(EXIT_SUCCESS);
    } else {

        /* Cierre del descriptor de salida en el padre */
		close(tuberia1[1]);

		/* Leer algo de la tubería... el saludo! */
		ssize_t nbytes = 0;
		do {
			nbytes = read(tuberia1[0], &numero, sizeof(int));
			if(nbytes == -1)
			{
				perror("read");
				exit(EXIT_FAILURE);
			}
		} while(nbytes != 0);

		while(wait(NULL) != -1);
    }

    pipe_status = pipe(tuberia2);
	if(pipe_status == -1)
	{
		perror("pipe");
		exit(EXIT_FAILURE);
	}

    pid = fork();
    if (pid < 0){
		perror("fork");
		exit(EXIT_FAILURE);
	} else if (pid == 0) {

        /* Cierre del descriptor de salida en el hijo */
		close(tuberia2[1]);
        
		/* Leer el numero en la tuberia */
		ssize_t nbytes = 0;
        int numero_recibido;
		
        nbytes = read(tuberia2[0], &numero_recibido, sizeof(int));
        if(nbytes == -1)
        {
            perror("read");
            exit(EXIT_FAILURE);
        }

        fprintf(pf ,"%d", numero_recibido);
		exit(EXIT_SUCCESS);
    } else {

        /* Cierre del descriptor de entrada en el padre */
		close(tuberia2[0]);

        ssize_t nbytes = write(tuberia2[1], &numero, sizeof(int));
		if(nbytes == -1)
		{
			perror("write");
			exit(EXIT_FAILURE);
		}

        wait(NULL);
        fclose(pf);
        exit(EXIT_SUCCESS);
    }
}
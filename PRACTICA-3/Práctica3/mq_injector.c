/**
 * @file mq_injector.c
 * @author Kevin de la Coba Malam (kevin.coba@estudiant.uam.es)
 *         Jose Manuel Freire     (jose.freire@estudiante.uam.es)
 * @brief Programa donde se envian mensajes de manera bloqueante hasta que 
 * se envie el contenido total del archivo.
 * @version 1
 * @date 2020-04-10
 * 
 * @copyright Copyright (c) 2020
 * 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>

#define MAX_SIZE 2

/**
 * @brief Estructura que define un mensaje
 * 
 */
typedef struct {
    char contenido[MAX_SIZE + 1]; /* +1 para indicar el final de la string */
} Mensaje;

int main(int argc, char **argv) {
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_curmsgs = 0,
        .mq_msgsize = sizeof(Mensaje)
    };

    FILE *pf = NULL;
    char **info = NULL;
    int trozos_archivo = 0;

    /* Comprobando argumentos */
    if (argc < 3 || argc > 3) {
        fprintf(stderr, "Argumentos invalidos, recuerde: ./injector <fichero> <nombre de la cola mensajes>\n");
        exit(EXIT_FAILURE);
    }

    /* Abrimos el archivo y lo leemos */
    pf = fopen(argv[1], "r");
    if (pf == NULL) {
        perror("fopen");
        exit(EXIT_FAILURE);
    }
    fseek(pf, 0, SEEK_SET);

    /* Reservamos memoria en la variable donde se guarda la información del fichero */
    info = (char**)malloc(sizeof(char*));
    if (info == NULL) {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    info[0] = (char*)malloc(MAX_SIZE*sizeof(char));
    if(info[0] == NULL) {
        free(info);
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    /* Dividiendo en trozos los el file */
    /* La comparacion es asi (== 1) porque esto significa que esta leyendo 2000 bytes, en el momento
       que no leamos 2000 bytes entonces habremos llegado al final del archivo */
    for (trozos_archivo = 0; fread(info[trozos_archivo], MAX_SIZE, 1, pf) == 1; trozos_archivo++) {
        info = realloc(info, (trozos_archivo + 2)*sizeof(char*));           /* Reallocamos el array de strings para añadir una string mas */
        info[trozos_archivo + 1] = (char*)malloc(MAX_SIZE*sizeof(char));    
        if (info[trozos_archivo + 1] == NULL) {
            for(int n = 0; n < trozos_archivo; n++) free(info[n]);
            free(info);
            exit(EXIT_FAILURE);
        }
    }

    /* Creamos la cola de mensaje */
    mqd_t cola = mq_open(argv[2],
        O_CREAT | O_WRONLY,
        S_IRUSR | S_IWUSR,
        &attributes);

    if (cola == (mqd_t)-1) {
        fprintf(stderr, "Error abriendo la cola\n");
        perror("mq_open");
        exit(EXIT_FAILURE);
    }

    /* Declaramos la cantidad de mensajes a enviar */
    Mensaje msg[trozos_archivo + 1];
    
    /* Copiamos todo el contenido */
    for (int i = 0; i < trozos_archivo; i++) strncpy(msg[i].contenido, info[i], MAX_SIZE*(sizeof(char)));

    /* Escribimos \0 al final de la string */
    for (int i = 0; i < trozos_archivo + 1; i++) msg[i].contenido[MAX_SIZE] = '\0';

    /* Enviamos los mensajes */
    for (int i = 0; i < trozos_archivo; i++) {
        if (mq_send(cola, (char *)&msg[i], sizeof(msg[i]), 1) == -1) {
            fprintf(stderr, "Error mandando el mensaje\n");
            perror("mq_send");
            for (int n = 0; n < trozos_archivo + 1; n++) free(info[n]);
            free(info);
            exit(EXIT_FAILURE);
        }
    }

    /* Enviamos un último mensaje */
    for (int i = 0; i < MAX_SIZE + 1; i++) msg[trozos_archivo].contenido[i] = '\0';
    if (mq_send(cola, (char *)&msg[trozos_archivo], sizeof(msg[trozos_archivo]), 1) == -1) {
            fprintf(stderr, "Error mandando el mensaje\n");
            perror("mq_send");
            for (int n = 0; n < trozos_archivo + 1; n++) free(info[n]);
            free(info);
            exit(EXIT_FAILURE);
        }

    /* Liberando memoria */
    for(int i = 0; i < trozos_archivo + 1; i++) free(info[i]);
    free(info);
    fclose(pf);

    mq_close(cola);
    mq_unlink(argv[2]);

    return 0;
}
/* SENDER */
#include <fcntl.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MQ_NAME "/mq_example"
#define N 33

typedef struct {
    int valor;
    char aviso[80];
} Mensaje;

int main(void) {
    struct mq_attr attributes = {
        .mq_flags = 0,
        .mq_maxmsg = 10,
        .mq_curmsgs = 0,
        .mq_msgsize = sizeof(Mensaje)
    };

    mqd_t queue = mq_open(MQ_NAME,
        O_CREAT | O_WRONLY, /* This process is only going to send messages */
        S_IRUSR | S_IWUSR, /* The user can read and write */
        &attributes);

    if (queue == (mqd_t)-1) {
        fprintf(stderr, "Error opening the queue\n");
        return EXIT_FAILURE;
    }

    Mensaje msg;
    msg.valor = 29;
    strcpy(msg.aviso, "Hola a todos");

    if (mq_send(queue, (char *)&msg, sizeof(msg), 1) == -1) {
        fprintf(stderr, "Error sending message\n");
        return EXIT_FAILURE;
    }

    /* Wait for input to end the program */
    fprintf(stdout, "Press any key to finish\n");
    getchar();

    mq_close(queue);
    mq_unlink(MQ_NAME);

    return EXIT_SUCCESS;
}

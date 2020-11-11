#define _POSIX_C_SOURCE 200112L

#include <errno.h>
#include <semaphore.h>
#include <stdio.h>
#include <time.h>
#include "utils.h"

int compute_log(int n) {
    int aux, ret;

    /* The logarithm is the position of the first bit. */
    aux = n;
    ret = 0;
    while (aux >>= 1) ++ret;

    return ret;
}

void clear_screen() {
    /* Prints clearing code. */
    printf("\x1B[1;1H\x1B[2J");
}

Status print_vector(int *data, int n_elements) {
    int i;

    if ((!(data)) || (n_elements <= 0)) {
        return ERROR;
    }

    for (i = 0; i < n_elements; i++) {
        printf("%d ", data[i]);
    }
    printf("\n");

    return OK;
}

Status plot_vector(int *data, int n_elements) {
    int i;

    if ((!(data)) || (n_elements <= 0)) {
        return ERROR;
    }

    /* The screen is cleaned before starting (any message should be below). */
    clear_screen();

    /* If the vector is large, then text mode. */
    if (n_elements > MAX_SIZE_PLOT) {
        return print_vector(data, n_elements);
    }

    /* If the vector is small, it is plotted using a bar diagram. */
    for (i = 0; i < n_elements; i++) {
        printf("\033[1;44m");
        printf("%*c", 2 * (data[i] + 1), ' ');
        printf("\033[0m\n");
    }

    return OK;
}

void fast_sleep(int nsec) {
    struct timespec time;

    if (nsec < 0) {
        nsec = 0;
    }

    time.tv_sec = 0;
    time.tv_nsec = nsec;

    nanosleep(&time, NULL);
}

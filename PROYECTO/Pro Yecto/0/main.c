#include <stdio.h>
#include <stdlib.h>
#include "sort.h"

int main(int argc, char *argv[]) {
    int n_levels, n_processes, delay;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <FILE> <N_LEVELS> <N_PROCESSES> [<DELAY>]\n", argv[0]);
        fprintf(stderr, "    <FILE> :        Data file\n");
        fprintf(stderr, "    <N_LEVELS> :    Number of levels (1 - %d)\n", MAX_LEVELS);
        fprintf(stderr, "    <N_PROCESSES> : Number of processes (1 - %d)\n", MAX_PARTS);
        fprintf(stderr, "    [<DELAY>] :     Delay (ms)\n");
        exit(EXIT_FAILURE);
    }

    n_levels = atoi(argv[2]);
    n_processes = atoi(argv[3]);
    if (argc > 4) {
        delay = 1e6 * atoi(argv[4]);
    }
    else {
        delay = 1e8;
    }

    return sort_single_process(argv[1], n_levels, n_processes, delay);
}

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
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "sort.h"

#define SHM_NAME "/shm"

int main(int argc, char *argv[]) {
    /*int n_levels, n_processes, delay;*/
    int shmid;
    Sort *sort_comp = NULL;

    if (argc < 4) {
        fprintf(stderr, "Usage: %s <FILE> <N_LEVELS> <N_PROCESSES> [<DELAY>]\n", argv[0]);
        fprintf(stderr, "    <FILE> :        Data file\n");
        fprintf(stderr, "    <N_LEVELS> :    Number of levels (1 - %d)\n", MAX_LEVELS);
        fprintf(stderr, "    <N_PROCESSES> : Number of processes (1 - %d)\n", MAX_PARTS);
        fprintf(stderr, "    [<DELAY>] :     Delay (ms)\n");
        exit(EXIT_FAILURE);
    }

    /* Abriendo la memoria compartida */
    shmid = shm_open(SHM_NAME, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
	if (shmid == -1) {
		perror("shm_open did not created the shared memory correctly.");
        return EXIT_FAILURE;
    }

    /* Cambiando el tamaño de la memoria compartida */
    if (ftruncate(shmid, sizeof(Sort)) == -1) {
		perror("ftruncate did not perform successfully.");
        shm_unlink(SHM_NAME);
        return EXIT_FAILURE;
	}

    /*Hacemos el mapeo de la memoria. */
    sort_comp = mmap(NULL, sizeof(*sort_comp), PROT_READ | PROT_WRITE, MAP_SHARED, shmid, 0);
	if(!sort_comp){
		perror("mmap: ");
		return 1;
	}

    if (init_sort(argv[1], sort_comp, atoi(argv[2]), atoi(argv[3]),atoi(argv[4])) == ERROR) {
        fprintf(stderr, "sort_single_process (shared memory) - init_sort\n");
        return ERROR;
    } 

    if (argc > 4) {
        sort_comp->delay = 1e6 * atoi(argv[4]);
    }
    else {
        sort_comp->delay = 1e8;
    }

    return sort_single_process(argv[1], sort_comp->n_levels, sort_comp->n_processes, sort_comp->delay);
    close(shmid);
}

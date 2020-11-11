/* This is only a code fragment, not a complete program... */
    int fd_shm = shm_open ("/shared", O_RDWR | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (fd_shm == -1) {
        if (errno == EEXIST) { /* Shared memory segment already exists */
            fd_shm = shm_open(SHM_NAME, O_RDWR, 0);
            if (fd_shm == -1) {
                fprintf(stderr, "Error opening the shared memory segment\n");
                return EXIT_FAILURE;
            }
        }
        else {
            fprintf(stderr, "Error creating the shared memory segment\n");
            return EXIT_FAILURE;
        }
    }
    else {
        printf ("Shared memory segment created\n");
    }

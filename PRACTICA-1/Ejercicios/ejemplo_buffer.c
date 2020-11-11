#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void)
{
	pid_t pid;
	FILE *pf = NULL;
	char *buffer;

	pf = fopen("file1.txt", "w");
	setvbuf(pf, buffer, _IONBF, 16);

	fprintf(pf, "Yo soy tu padre\n");

	pid = fork();
	if(pid <  0)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if(pid ==  0)
	{
		fprintf(pf, "Noooooo\n");
		exit(EXIT_SUCCESS);
	}

	fclose(pf);
	wait(NULL);
	exit(EXIT_SUCCESS);
}

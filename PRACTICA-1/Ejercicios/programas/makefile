all: ejercicio_arbol.o ejercicio_hilos.o ejercicio_pipes.o ejercicio_shell.o ejercicio_arbol ejercicio_hilos ejercicio_pipes ejercicio_shell

ejercicio_arbol.o:
	gcc -c ejercicio_arbol.c

ejercicio_arbol:
	gcc -o ejercicio_arbol ejercicio_arbol.o

ejercicio_hilos.o:
	gcc -c ejercicio_hilos.c -pthread

ejercicio_hilos:
	gcc -pthread -o ejercicio_hilos ejercicio_hilos.o

ejercicio_pipes.o:
	gcc -c ejercicio_pipes.c

ejercicio_pipes:
	gcc -o ejercicio_pipes ejercicio_pipes.o

ejercicio_shell.o:
	gcc -c ejercicio_shell.c

ejercicio_shell:
	gcc -o ejercicio_shell ejercicio_shell.o

clean: 
	rm -f *.o ejercicio_arbol ejercicio_shell ejercicio_hilos ejercicio_pipes numero_leido.txt
	
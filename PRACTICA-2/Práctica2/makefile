##########################################################################################################################################################################################
FLAGS = -g -o #-g Para debuggear
PFLAG = -g -pthread -o
EXE = ejercicio_lect_escr ejercicio_prottemp ejercicio_captura ejercicio_captura_mejorado ejercicio_sigset ejercicio_sem_signal ejercicio_prottemp_mejorado ejercicio_prottemp_mejorado_op ejercicio_kill ejercicio_alternar
##########################################################################################################################################################################################
all: gcc.o $(EXE)

gcc.o:
	gcc -g -c *.c

ejercicio_alternar:
	gcc $(PFLAG) ejercicio_alternar ejercicio_alternar.o

ejercicio_kill:
	gcc $(FLAGS) ejercicio_kill ejercicio_kill.o

ejercicio_prottemp:
	gcc $(FLAGS) ejercicio_prottemp ejercicio_prottemp.o

ejercicio_prottemp_mejorado:
	gcc $(PFLAG) ejercicio_prottemp_mejorado ejercicio_prottemp_mejorado.o

ejercicio_prottemp_mejorado_op:
	gcc $(PFLAG) ejercicio_prottemp_mejorado_op ejercicio_prottemp_mejorado_op.o

ejercicio_captura_mejorado:
	gcc $(FLAGS) ejercicio_captura_mejorado ejercicio_captura_mejorado.o

ejercicio_captura:
	gcc $(FLAGS) ejercicio_captura ejercicio_captura.o
	
ejercicio_sigset:
	gcc $(FLAGS) ejercicio_sigset ejercicio_sigset.o

ejercicio_sem_signal:
	gcc $(PFLAG) ejercicio_sem_signal ejercicio_sem_signal.o

ejercicio_lect_escr:
	gcc $(PFLAG) ejercicio_lect_escr ejercicio_lect_escr.o

clean:
	rm -f *.o $(EXE)

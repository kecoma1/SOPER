all: shm_producer shm_producer_file injector workers_pool shm_concurrence_solved

shm_concurrence_solved: shm_concurrence_solved.o
	gcc -pthread -g -o shm_concurrence_solved shm_concurrence_solved.o -lrt -lm

shm_concurrence_solved.o: 
	gcc -pthread -g -c shm_concurrence_solved.c -lm

shm_producer: shm_producer.o shm_consumer.o
	gcc  -pthread -g -o shm_producer shm_producer.o shm_consumer.o -lrt

shm_producer.o:
	gcc -pthread -lrt -g -c shm_producer.c

shm_consumer.o:
	gcc -lrt -pthread -g -c shm_consumer.c

shm_producer_file: shm_producer_file.o shm_consumer_file.o
	gcc  -pthread -g -o shm_producer_file shm_producer_file.o shm_consumer_file.o -lrt

shm_producer_file.o:
	gcc -pthread -lrt -g -c shm_producer_file.c

shm_consumer_file.o:
	gcc -lrt -pthread -g -c shm_consumer_file.c

injector: mq_injector.o
	gcc -g -o injector mq_injector.o -lrt 

mq_injector.o:
	gcc -g -lrt -c mq_injector.c

workers_pool: mq_workers_pool.o
	gcc -g -o workers_pool mq_workers_pool.o -lrt

mq_workers_pool.o:
	gcc -g -lrt -pthread -c mq_workers_pool.c

clean: 
	rm -f *.o shm_producer shm_producer_file injector workers_pool shm_concurrence_solved

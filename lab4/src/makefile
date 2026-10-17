CC = gcc
CFLAGS = -I.

LDFLAGS = -pthread

all : process_memory parallel_min_max parallel_sum _zombie

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c

utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)


process_memory : process_memory.c
	$(CC) -o process_memory process_memory.c $(CFLAGS)

parallel_sum : parallel_sum.c utils.o utils.h
	$(CC) -o parallel_sum parallel_sum.c utils.o $(CFLAGS) $(LDFLAGS)

_zombie : _zombie.c
	$(CC) -o _zombie _zombie.c $(CFLAGS)

clean :
	rm utils.o find_min_max.o parallel_min_max process_memory parallel_sum _zombie
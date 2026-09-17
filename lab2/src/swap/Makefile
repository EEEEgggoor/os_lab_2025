CC = gcc
CFLAGS = -Wall -g

TARGET = main
STATIC_LIB = libswap.a

all: $(TARGET)

swap.o: swap.c
	$(CC) $(CFLAGS) -o swap.o -c swap.c

$(STATIC_LIB): swap.o
	ar rcs $(STATIC_LIB) swap.o

$(TARGET): $(STATIC_LIB) main.o
	$(CC) main.o -o $(TARGET) -L. -lswap
	
clean:
	rm -f *.o *.a *.so $(TARGET)
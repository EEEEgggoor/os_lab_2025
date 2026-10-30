CC = gcc
CFLAGS = -g -pthread
LDFLAGS = -pthread

TARGET = main

.PHONY: all clean

all: $(TARGET)

mutex.o: mutex.c
	$(CC) $(CFLAGS) -c mutex.c -o mutex.o

$(TARGET): mutex.o
	$(CC) mutex.o -o $(TARGET) $(LDFLAGS)

clean:
	rm -f mutex.o $(TARGET)
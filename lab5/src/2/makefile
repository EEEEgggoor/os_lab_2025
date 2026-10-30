CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
LDFLAGS = -pthread

TARGET = main

.PHONY: all clean

all: $(TARGET)

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

$(TARGET): main.o
	$(CC) main.o -o $(TARGET) $(LDFLAGS)

clean:
	rm -f main.o $(TARGET)
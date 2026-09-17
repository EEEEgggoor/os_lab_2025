CC = gcc
CFLAGS = -Wall -g

TARGET = main
DYNAMIC_LIB = librevert_string.so

all: $(TARGET)

revert_string.o: revert_string.c
	$(CC) $(CFLAGS) -fPIC -c revert_string.c -o revert_string.o

$(DYNAMIC_LIB): revert_string.o
	$(CC) -shared -o $(DYNAMIC_LIB) revert_string.o

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

$(TARGET): main.o $(DYNAMIC_LIB)
	$(CC) main.o -o $(TARGET) -L. -lrevert_string -Wl,-rpath=.

clean:
	rm -f *.o *.a *.so $(TARGET)
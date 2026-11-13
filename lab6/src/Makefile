CC = gcc
CFLAGS = -Wall -pthread

all: server client

libcommon.a: common.c common.h
	$(CC) $(CFLAGS) -c common.c -o common.o
	ar rcs libcommon.a common.o

server: server.c libcommon.a
	$(CC) $(CFLAGS) server.c -L. -lcommon -o server

client: client.c libcommon.a
	$(CC) $(CFLAGS) client.c -L. -lcommon -o client

clean:
	rm -f server client common.o libcommon.a

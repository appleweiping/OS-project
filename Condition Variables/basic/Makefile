CC=gcc
CFLAGS=-Wall -ggdb2
LIBS=-lpthread

all: prodcons

prodcons: prodcons.c prodcons.h
	$(CC) $(CFLAGS) -o prodcons prodcons.c $(LIBS)

clean:
	rm -f prodcons

CC=gcc
CFLAGS=-Wall -ggdb2
LIBS=-lpthread

# The basic version shares arrivals.h, input.h and intersection_time.c with ../advanced
ADV=../advanced

all: intersection

intersection: intersection.c $(ADV)/intersection_time.c $(ADV)/intersection_time.h $(ADV)/arrivals.h $(ADV)/input.h
	$(CC) $(CFLAGS) -I$(ADV) -o intersection intersection.c $(ADV)/intersection_time.c $(LIBS)

clean:
	rm -f intersection

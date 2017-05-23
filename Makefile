CC=gcc
CFLAGS=-std=gnu99 -Wall -pedantic -Wextra -Werror
LDFLAGS=-lpthread -lrt

all: proj2

proj2: proj2.o functions.o

proj2.o: proj2.c functions.h

functions.o: functions.c functions.h

clean:
	rm -rf *.o proj2 proj2.out
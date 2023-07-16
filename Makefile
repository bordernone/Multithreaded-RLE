CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Wextra -Werror
LDFLAGS=-pthread -lm

.PHONY: all
all: nyuenc

nyuenc: nyuenc.o helper.o thread_pool.o

nyuenc.o: nyuenc.c helper.o thread_pool.o macros.h

helper.o: helper.c helper.h macros.h

thread_pool.o: thread_pool.c thread_pool.h macros.h

.PHONY: clean
clean:
	rm -f *.o nyuenc

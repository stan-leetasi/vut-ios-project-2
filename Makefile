CC=gcc
CFLAGS= -g -std=gnu99 -Wall -Wextra -Werror -pedantic

default: po-sim

po-sim: main.c
		$(CC) $(CFLAGS) main.c helper.c thread.c -o po-sim -lpthread

clean:
		rm -f po-sim *.out
		
CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I src/

mymalloc: src/malloc.c
	$(CC) $(CFLAGS) -o mymalloc src/malloc.c

valgrind: mymalloc
	valgrind --leak-check=full --error-exitcode=1 ./mymalloc

clean:
	rm -f mymalloc

.PHONY: valgrind clean

CC = gcc
CFLAGS = -Wall -Wextra -g -O2 -I src/


STRATEGY ?=

SRC = src/malloc.c
OBJ = $(SRC:.c=.o)




lib: $(SRC)
	$(CC) $(CFLAGS) $(STRATEGY) -fPIC -shared -o mymalloc.so $(SRC)


static: $(SRC)
	$(CC) $(CFLAGS) $(STRATEGY) -c -o src/malloc.o $(SRC)
	ar rcs libmymalloc.a src/malloc.o


test: static
	$(CC) $(CFLAGS) -o tests/test_basic tests/test_basic.c -L. -lmymalloc
	./tests/test_basic


bench-mine: static
	$(CC) -I src/ -O3 -o benchmarks/bench_mine benchmarks/bench.c -L. -lmymalloc
	./benchmarks/bench_mine

bench-system:
	$(CC) -I src/ -O3 -DUSE_SYSTEM_MALLOC -o benchmarks/bench_system benchmarks/bench.c
	./benchmarks/bench_system

bench: bench-mine bench-system


valgrind: static
	$(CC) $(CFLAGS) -o tests/test_basic tests/test_basic.c -L. -lmymalloc
	valgrind --leak-check=full --error-exitcode=1 ./tests/test_basic


clean:
	rm -f src/*.o *.so *.a tests/test_basic benchmarks/bench

.PHONY: lib static test bench valgrind clean

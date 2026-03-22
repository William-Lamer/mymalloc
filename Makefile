CC     = gcc
CFLAGS = -Wall -Wextra -g -O2 -I src/
LDLIBS = -lpthread

STRATEGY ?=
SRC = src/malloc.c


# Shared library for LD_PRELOAD
lib: $(SRC)
	$(CC) $(CFLAGS) $(STRATEGY) -fPIC -shared -o mymalloc.so $(SRC) src/shim.c $(LDLIBS)


# Static library for tests and benchmarks
static: $(SRC)
	$(CC) $(CFLAGS) $(STRATEGY) -c -o src/malloc.o $(SRC)
	ar rcs libmymalloc.a src/malloc.o


# Tests
test: static
	$(CC) $(CFLAGS) -o tests/test_basic tests/test_basic.c -L. -lmymalloc $(LDLIBS)
	./tests/test_basic


valgrind: static
	$(CC) $(CFLAGS) -o tests/test_basic tests/test_basic.c -L. -lmymalloc $(LDLIBS)
	valgrind --leak-check=full --error-exitcode=1 ./tests/test_basic


# Single-threaded benchmark (mymalloc vs glibc)
bench-mine: static
	$(CC) -I src/ -O3 -o benchmarks/bench_mine benchmarks/bench.c -L. -lmymalloc $(LDLIBS)
	./benchmarks/bench_mine

bench-system:
	$(CC) -I src/ -O3 -DUSE_SYSTEM_MALLOC -o benchmarks/bench_system benchmarks/bench.c
	./benchmarks/bench_system

bench: bench-mine bench-system


# Multithreaded benchmark (mymalloc vs glibc)
bench-threaded-mine: static
	$(CC) -I src/ -O3 -o benchmarks/bench_threaded_mine benchmarks/bench_threaded.c -L. -lmymalloc $(LDLIBS)
	./benchmarks/bench_threaded_mine

bench-threaded-system:
	$(CC) -I src/ -O3 -DUSE_SYSTEM_MALLOC -o benchmarks/bench_threaded_system benchmarks/bench_threaded.c $(LDLIBS)
	./benchmarks/bench_threaded_system

bench-threaded: bench-threaded-mine bench-threaded-system


# Run everything
all: test bench bench-threaded


clean:
	rm -f src/*.o *.so *.a
	rm -f tests/test_basic
	rm -f benchmarks/bench_mine benchmarks/bench_system
	rm -f benchmarks/bench_threaded_mine benchmarks/bench_threaded_system

.PHONY: lib static test valgrind bench-mine bench-system bench \
        bench-threaded-mine bench-threaded-system bench-threaded all clean

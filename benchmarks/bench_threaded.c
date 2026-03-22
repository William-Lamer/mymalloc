#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>


#ifdef USE_SYSTEM_MALLOC
    #define BENCH_MALLOC malloc
    #define BENCH_FREE   free
    #define ALLOCATOR_NAME "glibc malloc"
#else
    #include "../src/malloc.h"
    #define BENCH_MALLOC my_malloc
    #define BENCH_FREE   my_free
    #define ALLOCATOR_NAME "mymalloc"
#endif

#define NUM_RUNS        5
#define POOL_SIZE       512
#define ITERATIONS      1000000
#define MAX_SIZE        4096
#define MIN_SIZE        16

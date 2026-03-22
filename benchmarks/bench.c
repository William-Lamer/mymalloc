#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


//Compile with glibc using -DUSE_SYSTEM_MALLOC
//Default uses custom built allocator


#ifdef USE_SYSTEM_MALLOC
    #define BENCH_MALLOC malloc
    #define BENCH_FREE free
    #define ALLOCATOR_NAME "glibc malloc"
#else
    #include "../src/malloc.h"
    #define BENCH_MALLOC my_malloc
    #define BENCH_FREE my_free
    #define ALLOCATOR_NAME "mymalloc"
#endif

#define NUM_RUNS        5
#define POOL_SIZE       512
#define ITERATIONS      1000000
#define MAX_SIZE        4096
#define MIN_SIZE        16



//Precalculate our random inputs so we dont waste time

static int precalculate_slots[ITERATIONS];
static size_t precalculate_sizes[ITERATIONS];

static void setup_random_values() {
    srand(42); //keep it fixed 
    for (int i = 0; i < ITERATIONS; i++) {
        precalculate_slots[i] = rand() % POOL_SIZE;
        precalculate_sizes[i] = MIN_SIZE + (rand() % (MAX_SIZE - MIN_SIZE));
    }
}

static double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}


static double benchmark() {
    void *pool[POOL_SIZE] = {0};

    double start = get_time_seconds();

    for (int i = 0; i < ITERATIONS; i++) {
        int slot = precalculate_slots[i];
        size_t size = precalculate_sizes[i];


        if (pool[slot] == NULL) {
            pool[slot] = BENCH_MALLOC(size);

            if (pool[slot]) {
                memset(pool[slot], 0xAB, size);
            }
        } else {
            BENCH_FREE(pool[slot]);
            pool[slot] = NULL;
        }
    }

    double elapsed = get_time_seconds() - start;

    //remove everything left
    for (int i = 0; i < POOL_SIZE; i++) {
        if (pool[i]) {
            BENCH_FREE(pool[i]);
            pool[i] = NULL;
        }
    }
    return elapsed;
}







int main() {
    setup_random_values();

    printf("\n%-12s | iterations: %d | runs: %d | sizes: %d-%d bytes\n",
           ALLOCATOR_NAME, ITERATIONS, NUM_RUNS, MIN_SIZE, MAX_SIZE);


    double total = 0.0;
    double best = 1e9;
    double worst = 0.0;

    for (int i = 0; i < NUM_RUNS; i++) {
        double elapsed = benchmark();
        total += elapsed;
        if (elapsed < best) best = elapsed;
        if (elapsed > worst) worst = elapsed;

        printf("   Run %d: %.4fs | %.0f ops/sec\n",
               i+1, elapsed, ITERATIONS / elapsed);
    }

    double avg = total / NUM_RUNS;
    double throughput = ITERATIONS / avg;

    printf("------Final Results------\n");
    printf(" Average : %.4fs | %.0f ops/sec\n", avg, throughput);
    printf(" Best    : %.4fs | %.0f ops/sec\n", best, ITERATIONS / best);
    printf(" worst   : %.4fs | %0.f ops/sec\n", worst, ITERATIONS / worst);
    printf("\n");

    return 0;
}







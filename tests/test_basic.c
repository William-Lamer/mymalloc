#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../src/malloc.h"



#define GIGA_TEST_ITERS 10000
#define POOL_SIZE 64
#define MAX_ALLOC_SIZE 512


void test_data_integrity(){
    printf("\nRunning data integrity test...\n");

    int *arr1 = my_malloc(100 * sizeof(int));
    assert(heap_check() == 1);
    int *arr2= my_malloc(200 * sizeof(int));
    assert(heap_check() == 1);
    int *arr3 = my_malloc(300 * sizeof(int));
    assert(heap_check() == 1);

    assert(arr1 != NULL);
    assert(arr2 != NULL);
    assert(arr3 != NULL);


    //Fill with data
    for (int i = 0; i < 100; i++) arr1[i] = 1;
    for (int i = 0; i < 200; i++) arr2[i] = 2;
    for (int i = 0; i < 300; i++) arr3[i] = 3;

    //Verify the data hasn't been overwritten
    for (int i = 0; i < 100; i++) assert(arr1[i] == 1);
    for (int i = 0; i < 200; i++) assert(arr2[i] == 2);
    for (int i = 0; i < 300; i++) assert(arr3[i] == 3);

    my_free(arr1);
    assert(heap_check() == 1);
    my_free(arr2);
    assert(heap_check() == 1);
    my_free(arr3);
    assert(heap_check() == 1);

    printf("Data Integrity Test Passed!\n");
}


void test_reuse(){
    printf("Running reuse test...\n");

    void *a = my_malloc(128);
    assert(heap_check() == 1);
    void *b = my_malloc(128);
    assert(heap_check() == 1);
    void *c = my_malloc(128);
    assert(heap_check() == 1);

    my_free(b);
    assert(heap_check() == 1);

    void *d = my_malloc(64);
    assert(heap_check() == 1);
    assert(d == b); //check if the pointers are identical
    

    my_free(a);
    assert(heap_check() == 1);
    my_free(d);
    assert(heap_check() == 1);
    my_free(c);
    assert(heap_check() == 1);

    printf("Reuse test passed!\n");

}



void test_coalesce(){
    printf("Running coalesce test...\n");

    void *p1 = my_malloc(100);
    assert(heap_check() == 1);
    void *p2 = my_malloc(100);
    assert(heap_check() == 1);
    void *p3 = my_malloc(100);
    assert(heap_check() == 1);

    my_free(p2);
    assert(heap_check() == 1);
    my_free(p1);
    assert(heap_check() == 1);
    my_free(p3);
    assert(heap_check() == 1);

    void *p4 = my_malloc(300);
    assert(heap_check() == 1);
    //should fit and not need to request more memory
    assert(p4 == p1); //they should point to the same address

    my_free(p4);
    assert(heap_check() == 1);


    printf("Coalescing test passed!\n");
}



void test_edge_cases(){
    printf("Running edge cases test...\n");

    //Allocating 0 bytes
    void *zero_ptr = my_malloc(0);
    assert(heap_check() == 1);
    assert(zero_ptr == NULL);

    //Freeing NULL
    my_free(NULL);
    assert(heap_check() == 1);

    //Allocating a huge size
    size_t huge_size = 1<<20; //cant be too big or valgrind will reject. But still works
    void *huge_ptr = my_malloc(huge_size);
    assert(heap_check() == 1);
    assert(huge_ptr != NULL);

    //Trying to write inside the block
    ((char*)huge_ptr)[0] = 'A';
    ((char*)huge_ptr)[huge_size - 1] = 'B';

    assert(((char*)huge_ptr)[0] == 'A');
    assert(((char*)huge_ptr)[huge_size - 1] == 'B');

    my_free(huge_ptr);
    assert(heap_check() == 1);

    printf("Edge cases test passed!");
}





// keeps a pool of pointers, randomly malloc and free over x times. 
// write a known pattern in each block, verify the pattern then heap_check then free. 
void giga_test(){
    printf("\nRunning GIGA test at %d iterations...\n", GIGA_TEST_ITERS);

    void *pool[POOL_SIZE] = {0};
    size_t sizes[POOL_SIZE] = {0}; // the size of each allocation

    srand(42); //keep the same, dont modify

    for (int i=0; i < GIGA_TEST_ITERS; i++) {
        //testing 
        




        int slot = rand() % POOL_SIZE;

        if (pool[slot] == NULL) {
            // the slot is free so we allocate
            size_t size = (rand() % MAX_ALLOC_SIZE) + 1;
            pool[slot] = my_malloc(size);
            sizes[slot] = size;

            assert(pool[slot] != NULL);

            memset(pool[slot], (slot+1), size); //use +1 so slot 0 won't be 0. Could hide bugs
        } else {
            //slot is allocated 
            //verifiy the pattern

            unsigned char expected = (unsigned char)(slot + 1);
            unsigned char *data = (unsigned char *)pool[slot];

            for (size_t j = 0; j < sizes[slot]; j++) {
                if (data[j] != expected) {
                    printf("Corruption at slot %d, byte %zu: expected %d and got %d\n", slot, j, expected, data[j]);
                    assert(0);
                }
            }

            my_free(pool[slot]);
            pool[slot] = NULL;
            sizes[slot] = 0;

        }

        //verify heap integrity after every iteration
        assert(heap_check() == 1);
    }

    //When finished, free everything remaining
    for (int i = 0; i < POOL_SIZE; i++) {
        if (pool[i] != NULL) {
            my_free(pool[i]);
            pool[i] = NULL;
        }
    }
    assert(heap_check() == 1);
    printf("Giga test passed!\n");
}




int main() {
    printf("\nStarting custom basic tests...\n");



    test_data_integrity();


    test_reuse();
    test_coalesce();
    test_edge_cases();
    giga_test();


    printf("\n-------------------");
    printf("\n-ALL TEST PASSED!!-");
    printf("\n-------------------");
}
















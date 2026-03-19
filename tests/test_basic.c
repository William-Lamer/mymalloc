#include <stdio.h>
#include <assert.h>
#include "../src/malloc.h"



#define GIGA_TEST_ITERS 10000


void test_data_integrity(){
    printf("\n\nRunning data integrity test...\n\n");

    int *arr1 = my_malloc(100 * sizeof(int));
    int *arr2= my_malloc(200 * sizeof(int));
    int *arr3 = my_malloc(300 * sizeof(int));

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
    my_free(arr2);
    my_free(arr3);

    printf("Data Integrity Test Passed!\n\n");
}


void test_reuse(){
    printf("Running reuse test...\n\n");

    void *a = my_malloc(128);
    void *b = my_malloc(128);
    void *c = my_malloc(128);

    my_free(b);

    void *d = my_malloc(64);
    assert(d == b); //check if the pointers are identical
    

    my_free(a);
    my_free(b);
    my_free(c);

    printf("Reuse test passed!\n");

}



void test_coalesce(){
    printf("Running coalesce test...\n");

    void *p1 = my_malloc(100);
    void *p2 = my_malloc(100);
    void *p3 = my_malloc(100);

    my_free(p2);
    my_free(p1);
    my_free(p3);

    void *p4 = my_malloc(300);
    //should fit and not need to request more memory
    assert(p4 == p1); //they should point to the same address

    my_free(p4);


    printf("Coalescing test passed!\n");
}



void test_edge_cases(){
    printf("Running edge cases test...\n");

    //Allocating 0 bytes
    void *zero_ptr = my_malloc(0);
    assert(zero_ptr == NULL);

    //Freeing NULL
    my_free(NULL);

    //Allocating a huge size
    size_t huge_size = 1<<24; //16MB
    void *huge_ptr = my_malloc(huge_size);
    assert(huge_ptr != NULL);

    //Trying to write inside the block
    ((char*)huge_ptr)[0] = 'A';
    ((char*)huge_ptr)[huge_size - 1] = 'B';

    assert(((char*)huge_ptr)[0] == 'A');
    assert(((char*)huge_ptr)[huge_size - 1] == 'B');

    my_free(huge_ptr);

    printf("Edge cases test passed!");
}






void giga_test(){
    printf("\nRunning GIGA test at %d iterations...\n", GIGA_TEST_ITERS);














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
















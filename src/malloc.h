#ifndef MY_MALLOC_H
#define MY_MALLOC_H

#include <stddef.h>

void *my_malloc(size_t size);
void my_free(void *ptr);
void print_heap(); 

#endif

//This file is used for LD_PRELOAD


#include <stddef.h>
#include "malloc.h"

void *malloc(size_t size)           {return my_malloc(size);}
void free(void *ptr)                {my_free(ptr);}
void *realloc(void *ptr, size_t size)   {return my_realloc(ptr, size);}
void *calloc(size_t num, size_t size)   {return my_calloc(num , size);}

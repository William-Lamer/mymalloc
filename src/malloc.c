#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#define WSIZE sizeof(size_t) // 8 bytes on 64bit
#define DSIZE (2 * WSIZE)      // 16 bytes on 64bit
#define CHUNKSIZE (1 << 12)  // 4096 bytes

#define MAX(x, y)        ((x) > (y))? (x) : (y)

// Pack the size and the allocated bit together into one word
#define PACK(size, alloc) ((size) | (alloc))

// Read/write a word at the address p
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

// Extract the size and the allocated bit from address p
#define GET_SIZE(p) (GET(p) & ~(DSIZE - 1)) //total size including metadata
#define GET_ALLOC(p) (GET(p) & 0x1)

// Block Navigation Macros -> The pointer points to the payload. 
#define HEADER(ptr) ((char *)(ptr) - WSIZE)
#define FOOTER(ptr) ((char *)(ptr) + GET_SIZE(HEADER(ptr)) - DSIZE)

// Get addresses of next block and previous block
#define NEXT_BLOCK(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr) - WSIZE))
#define PREV_BLOCK(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - DSIZE))


//Function declarations
void *request_from_os(size_t size);
void print_heap();
static void *my_malloc(size_t size);
void my_free(void *ptr);
static void *coalesce(void *ptr);
int heap_init();
static void *extend_heap(size_t words);

static void *heap_start = NULL;




int heap_init(){

    //Create the initial empty heap
    char *p = request_from_os(4 * WSIZE);
    if (!p) return -1;

    PUT(p, 0); //Alignment padding
    PUT(p + (1*WSIZE), PACK(DSIZE, 1)); //Prologue header
    PUT(p + (2*WSIZE), PACK(DSIZE, 1)); //Prologue footer
    PUT(p + (3*WSIZE), PACK(0, 1)); //Epilogue header
    

    heap_start = p + (2 * WSIZE);


    //Extend the empty heap with a free block of size CHUNKSIZE


    return 0;
}

static void *extend_heap(size_t words){

    char *ptr;
    size_t size;

    //Need to allocate an even number of words to maintain allignment
    if (words % 2){
        size = (words + 1) * WSIZE;
    } else {
        size = words * WSIZE;
    }

    if ((ptr = request_from_os(size)) == NULL) return NULL;

    //Initialize the header and footer of the new free block.


    
    
    




    //coalesce if the previous block was free
    coalesce(ptr);


    return 0;
}






// Request bytes from the OS, returns pointer or NULL on fail.
void *request_from_os(size_t size) {
    void *block = sbrk(size);
    if (block == (void *)-1)
        return NULL;
    return block;
}


void print_heap() {
    if (!heap_start) {
        printf("Heap is not initialized\n");
        return;
    }

    char *ptr = heap_start;
    int block_num = 0;
    printf("\n--- HEAP STATE ---\n");
    while (GET_SIZE(HEADER(ptr)) > 0) {
        printf("Block %d | addr: %p | size %zu | %s\n", block_num, ptr,
               GET_SIZE(HEADER(ptr)),
               GET_ALLOC(HEADER(ptr)) ? "ALLOCATED" : "FREE");
        ptr = NEXT_BLOCK(ptr);

        block_num++;
    }
    printf("--------------\n\n");
}


void *my_malloc(size_t size) {
    if (!heap_start && heap_init() < 0) return NULL;


    size_t block_size;
    size_t extendsize;

    if (size == 0) return NULL;

    //Align the requested size.
    if (size <= DSIZE){
        block_size = 2*DSIZE;
    } else {
        block_size = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    }

    //request the OS for the memory
    char *block = request_from_os(block_size);
    if (!block) return NULL;

    //Write the header and footer
    PUT(block, PACK(block_size, 1)); //header
    PUT(block + block_size - WSIZE, PACK(block_size, 1)); //footer
    
    //return pointer to the block (just after the header)
    void *ptr = block + WSIZE;

    if (!heap_start) heap_start  = ptr;

    return ptr;
}



void my_free(void *ptr){
    size_t size = GET_SIZE(HEADER(ptr));
    
    PUT(HEADER(ptr), PACK(size, 0));
    PUT(FOOTER(ptr), PACK(size, 0));
    coalesce(ptr); 
}




static void *coalesce(void *ptr){
    return 0; 
}









int main() {
    //Basic allocation testing 
    int *arr = my_malloc(10 * sizeof(int));
    for (int i=0; i<10; i++) arr[i] = i;
    for (int i=0; i<10; i++) printf("%d ", arr[i]);
    printf("\n\n");


    //check that the header is correct
    printf("Block size in header: %zu\n", GET_SIZE(HEADER(arr)));
    printf("Alloc bit in header:  %zu\n", GET_ALLOC(HEADER(arr)));

    //Check that the footer matches the header
    printf("Footer matches header: %s\n", GET(HEADER(arr)) == GET(FOOTER(arr)) ? "YES" : "NO");



    //testing another allocation
    int *arr2 = my_malloc(100 * sizeof(int));

    for (int i=0; i<10; i++) arr[i] = i;

   

    print_heap();


    return 0;
}

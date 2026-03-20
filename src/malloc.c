#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>

//Block sizes are the total block sizes (including header and footer)


#define WSIZE sizeof(size_t) // 8 bytes on 64bit
#define DSIZE (2 * WSIZE)    // 16 bytes on 64bit

#define EXTEND_HEAP_AMOUNT  (1 << 12) // 4096 bytes
#define MINIMUM_BLOCK_SIZE  (DSIZE)

#define MAX(x, y) (((x) > (y)) ? (x) : (y))

// Pack the size and the allocated bit together into one word
#define PACK(size, alloc) ((size) | (alloc))

// Read/write a word at the address p
#define GET(p) (*(size_t *)(p))
#define PUT(p, val) (*(size_t *)(p) = (val))

// Extract the size and the allocated bit from address p
#define GET_SIZE(p) (GET(p) & ~(DSIZE - 1)) // total size including metadata
#define GET_ALLOC(p) (GET(p) & 0x1)

// Block Navigation Macros -> The pointer points to the payload.
#define HEADER(ptr) ((char *)(ptr) - WSIZE)
#define FOOTER(ptr) ((char *)(ptr) + GET_SIZE(HEADER(ptr)) - DSIZE)

// Get addresses of next block and previous block
#define NEXT_BLOCK(ptr) ((char *)(ptr) + GET_SIZE((char *)(ptr) - WSIZE))
#define PREV_BLOCK(ptr) ((char *)(ptr) - GET_SIZE((char *)(ptr) - DSIZE))

// Function declarations
void *request_from_os(size_t size);
void print_heap(void);
void *my_malloc(size_t size);
void my_free(void *ptr);
static void *coalesce(void *ptr);
int heap_init(void);
static void *extend_heap(size_t bytes);
static void *find_first_fit(size_t block_size_requested);
static void carve(void *ptr, size_t block_size_requested);
static int check_block(void *ptr);  //validates one block
void *my_calloc(size_t num, size_t size);
int heap_check(void); //validates the entire heap
static int heap_error(const char *msg, void *ptr);




static void *heap_start = NULL;

int heap_init(void) {

    // Create the initial empty heap
    char *p = request_from_os(4 * WSIZE);
    if (!p)
        return -1;

    PUT(p, 0);                            // Alignment padding
    PUT(p + (1 * WSIZE), PACK(DSIZE, 1)); // Prologue header
    PUT(p + (2 * WSIZE), PACK(DSIZE, 1)); // Prologue footer
    PUT(p + (3 * WSIZE), PACK(0, 1));     // Epilogue header

    heap_start = p + (2 * WSIZE);

    // Extend the empty heap with a free block of size CHUNKSIZE
    if (extend_heap(EXTEND_HEAP_AMOUNT) == NULL) return -1;

    return 0;
}

static void *extend_heap(size_t bytes) {

    char *ptr;
    size_t size;

    // Need to allocate an even number of words to maintain allignment
    size_t words = bytes / WSIZE;

    if (words % 2) {
        size = (words + 1) * WSIZE;
    } else {
        size = words * WSIZE;
    }

    if ((ptr = request_from_os(size)) == NULL)
        return NULL;

    // Initialize the header and footer of the new free block.
    PUT(HEADER(ptr), PACK(size, 0));
    PUT(FOOTER(ptr), PACK(size, 0));

    // Move the epilogue header
    PUT(HEADER(NEXT_BLOCK(ptr)), PACK(0, 1));

    // coalesce if the previous block was free
    return coalesce(ptr);
}

// Request bytes from the OS, returns pointer or NULL on fail.
void *request_from_os(size_t size) {
    void *block = sbrk(size);
    if (block == (void *)-1)
        return NULL;
    return block;
}

void print_heap(void) {
    if (!heap_start) {
        printf("Heap is not initialized\n");
        return;
    }

    char *ptr = NEXT_BLOCK(heap_start);
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

static void *find_first_fit(size_t block_size_requested) {
    // Get the pointer to the payload of the first real block
    void *ptr = NEXT_BLOCK(heap_start);

    while (GET_SIZE(HEADER(ptr)) > 0){
        //Check if the block is free and big enough
        if (!GET_ALLOC(HEADER(ptr)) && (GET_SIZE(HEADER(ptr)) >= block_size_requested)){
            carve(ptr, block_size_requested);
            return ptr;
            
        }
        ptr = NEXT_BLOCK(ptr);
    }
    return NULL;
}

static void carve(void *ptr, size_t block_size_requested){
    
    size_t size_block_found = GET_SIZE(HEADER(ptr));
    
    //Return if the block is not big enough to carve with space leftover
    if (size_block_found <= block_size_requested + MINIMUM_BLOCK_SIZE){
        //block is too small to divide
        PUT(HEADER(ptr), PACK(size_block_found, 1));
        PUT(FOOTER(ptr), PACK(size_block_found, 1));
        return;
    }

    //Update the header and footer for its new size
    PUT(HEADER(ptr), PACK(block_size_requested, 1));
    PUT(FOOTER(ptr), PACK(block_size_requested, 1)); //also creates the footer

    //Update the values for the new empty block created
    void *new_ptr = NEXT_BLOCK(ptr);
    PUT(HEADER(new_ptr), PACK(size_block_found - block_size_requested, 0));
    PUT(FOOTER(new_ptr), PACK(size_block_found - block_size_requested, 0)); 
}


void *my_malloc(size_t size) {
    if (!heap_start && heap_init() < 0) return NULL;

    if (size == 0) return NULL;

    size_t block_size;
    // Align the requested size.
    if (size <= DSIZE) {
        block_size = 2 * DSIZE;
    } else {
        block_size = DSIZE * ((size + (DSIZE) + (DSIZE - 1)) / DSIZE);
    }


    // Find the first free block
    char *block = find_first_fit(block_size);
    if (block) return block;


    //If no fit is found, request more space from the OS
    size_t extendsize;
    extendsize = MAX(block_size, EXTEND_HEAP_AMOUNT);
    void *new_block = extend_heap(extendsize);
    if (!new_block) return NULL;

    carve(new_block, block_size);
    return new_block;
}

void my_free(void *ptr) {
    if (ptr == NULL) return;

    size_t size = GET_SIZE(HEADER(ptr));

    PUT(HEADER(ptr), PACK(size, 0));
    PUT(FOOTER(ptr), PACK(size, 0));
    coalesce(ptr);
}

static void *coalesce(void *ptr) { 

    size_t previous_block_allocated = GET_ALLOC(FOOTER(PREV_BLOCK(ptr)));
    size_t next_block_allocated = GET_ALLOC(HEADER(NEXT_BLOCK(ptr)));

    size_t size_middle_block = GET_SIZE(HEADER(ptr));
    size_t size_previous_block = GET_SIZE(FOOTER(PREV_BLOCK(ptr)));
    size_t size_next_block = GET_SIZE(HEADER(NEXT_BLOCK(ptr)));
    size_t new_size;

    //If both are not free
    if (previous_block_allocated && next_block_allocated) return ptr;

    //If only the previous block is free
    if (!previous_block_allocated && next_block_allocated){
        new_size = size_middle_block + size_previous_block;

        PUT(HEADER(PREV_BLOCK(ptr)), PACK(new_size, 0));
        PUT(FOOTER(ptr), PACK(new_size, 0));

        return PREV_BLOCK(ptr);
    }


    //if only the next block is free
    if (previous_block_allocated && !next_block_allocated){
        new_size = size_middle_block + size_next_block;

        PUT(HEADER(ptr), PACK(new_size, 0));
        PUT(FOOTER((ptr)), PACK(new_size, 0));

        return ptr;
    }

    //if both blocks are free
    if (!previous_block_allocated && !next_block_allocated){
        new_size = size_middle_block + size_previous_block + size_next_block;

        PUT(HEADER(PREV_BLOCK(ptr)), PACK(new_size, 0));
        PUT(FOOTER(NEXT_BLOCK(ptr)), PACK(new_size, 0));

        return PREV_BLOCK(ptr);
    }
    return NULL;
}



void *my_calloc(size_t num, size_t size) {
    //Need to add check to make sure size_t doesnt overflow
    size_t total = num * size;
    void *ptr = my_malloc(total);

    if (!ptr) return NULL;

    memset(ptr, 0, total);
    return ptr;
}



static int heap_error(const char *msg, void *ptr) {
    fprintf(stderr, "HEAP CHECK FAILED: %s (block=%p)", msg, ptr);
    return 0;
}



static int check_block(void *ptr) {
    size_t header = GET(HEADER(ptr));
    size_t footer = GET(FOOTER(ptr));
    size_t size = GET_SIZE(HEADER(ptr));


    //Alignment
    if ((uintptr_t)ptr % DSIZE != 0) {
        return heap_error("paylod pointer is not 16 byte aligned", ptr);
    }
    //Valid size, if too small and if not a multiple of.
    if (size < 2 * WSIZE){
        return heap_error("block size is too small", ptr);
    }

    if (size % DSIZE != 0) {
        return heap_error("block size is not a multiple of DSIZE", ptr);
    }

    //Header and footer matches
    if (header != footer) {
        return heap_error("header and footer do not match", ptr);
    }

    return 1;
}





int heap_check(void) {
    if (!heap_start) {
        return heap_error("heap not initialized", NULL);
    }

    char *base = (char *)heap_start - 2 * WSIZE; //start of padding before prologue header
    
    

    //check header
    base += WSIZE;
    assert(GET_SIZE(base) == DSIZE);
    assert(GET_ALLOC(base) == 1);
    
    //move and check header
    base += WSIZE;
    assert(GET_SIZE(base) == DSIZE);
    assert(GET_ALLOC(base) == 1);

    //loop through all blocks and check
    char *ptr = NEXT_BLOCK(heap_start);
    while (GET_SIZE(HEADER(ptr)) > 0) {






    }




    return 1;
}















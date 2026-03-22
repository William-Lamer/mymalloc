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
#define MINIMUM_BLOCK_SIZE  (2 * DSIZE) //Header, footer + minimum payload size

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


#define NEXT_FREE(ptr) (*(void **)(ptr))
#define PREV_FREE(ptr) (*(void **)((char *)(ptr) + WSIZE))



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
void *my_realloc(void *ptr, size_t size);
int heap_check(void); //validates the entire heap
static int heap_error(const char *msg, void *ptr);
static void insert_free_block(void *ptr);
static void remove_free_block(void *ptr);
static int check_free_list(void);




static void *heap_start = NULL;
static void *free_list_head = NULL;

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
    void *ptr = free_list_head;
    // Find the first free block

    while (ptr != NULL) {
        if (GET_SIZE(HEADER(ptr)) >= block_size_requested) {
            return ptr;
        }
        ptr = NEXT_FREE(ptr);
    }
    return NULL;
}

static void carve(void *ptr, size_t block_size_requested){
    
    size_t size_block_found = GET_SIZE(HEADER(ptr));

    remove_free_block(ptr);
    
    //Return if the block is not big enough to carve with space leftover
    if (size_block_found < block_size_requested + MINIMUM_BLOCK_SIZE){
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
    size_t rest = size_block_found - block_size_requested;

    PUT(HEADER(new_ptr), PACK(rest, 0));
    PUT(FOOTER(new_ptr), PACK(rest, 0)); 

    insert_free_block(new_ptr);
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

    void *block = find_first_fit(block_size);
    if (block == NULL) {
        size_t extendsize = MAX(block_size, EXTEND_HEAP_AMOUNT);
        block = extend_heap(extendsize);
        if (block == NULL) return NULL;
    }

    carve(block, block_size);
    return block;
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
    if (previous_block_allocated && next_block_allocated) {
        insert_free_block(ptr);
        return ptr;
    }

    //If only the previous block is free
    if (!previous_block_allocated && next_block_allocated){
        remove_free_block(PREV_BLOCK(ptr));

        new_size = size_middle_block + size_previous_block;

        PUT(HEADER(PREV_BLOCK(ptr)), PACK(new_size, 0));
        PUT(FOOTER(ptr), PACK(new_size, 0));

        insert_free_block(PREV_BLOCK(ptr));
        return PREV_BLOCK(ptr);
    }


    //if only the next block is free
    if (previous_block_allocated && !next_block_allocated){
        remove_free_block(NEXT_BLOCK(ptr));
        new_size = size_middle_block + size_next_block;

        PUT(HEADER(ptr), PACK(new_size, 0));
        PUT(FOOTER((ptr)), PACK(new_size, 0));

        insert_free_block(ptr);
        return ptr;
    }

    //if both blocks are free
    if (!previous_block_allocated && !next_block_allocated){
        remove_free_block(PREV_BLOCK(ptr));
        remove_free_block(NEXT_BLOCK(ptr));

        new_size = size_middle_block + size_previous_block + size_next_block;

        PUT(HEADER(PREV_BLOCK(ptr)), PACK(new_size, 0));
        PUT(FOOTER(NEXT_BLOCK(ptr)), PACK(new_size, 0));

        insert_free_block(PREV_BLOCK(ptr));
        return PREV_BLOCK(ptr);
    }
    return NULL;
}



void *my_calloc(size_t num, size_t size) {
    if (num == 0 || size == 0) {
        return my_malloc(0);
    }
    // check if it overflows
    if (size > SIZE_MAX / num) {
        return NULL;
    }

    size_t total = num * size;
    void *ptr = my_malloc(total);
    if (ptr == NULL) return NULL;

    memset(ptr, 0, total);
    return ptr;
}

void *my_realloc(void *ptr, size_t size) {
    if (ptr == NULL) { //just behaves like normal malloc
        return my_malloc(size);
    }

    if (size == 0) { //is supposed to free it
        my_free(ptr);
        return NULL;
    }

    size_t old_block_size = GET_SIZE(HEADER(ptr));
    size_t new_block_size;
    if (size <= DSIZE) {
        new_block_size = 2 * DSIZE;
    } else {
        new_block_size = DSIZE * ((size + DSIZE + (DSIZE -1)) / DSIZE);
    }


    //Case 1: Current block is already large enough
    if (old_block_size >= new_block_size) {
        return ptr;
    }

    //Case 2: If the next block is free and we can grow into it 
    void *next = NEXT_BLOCK(ptr);
    if (!GET_ALLOC(HEADER(next))) {
        size_t next_size = GET_SIZE(HEADER(next));
        size_t combined_size = old_block_size + next_size;

        if (combined_size >= new_block_size) {
            remove_free_block(next);

            size_t rest = combined_size - new_block_size;

            if (rest >= MINIMUM_BLOCK_SIZE) {
                //Expand the allocated block
                PUT(HEADER(ptr), PACK(new_block_size, 1));
                PUT(FOOTER(ptr), PACK(new_block_size, 1));

                //create the new free block that remains
                void *split = NEXT_BLOCK(ptr);
                PUT(HEADER(split), PACK(rest, 0));
                PUT(FOOTER(split), PACK(rest, 0));
                insert_free_block(split);
            } else {
                //use the entire space
                PUT(HEADER(ptr), PACK(combined_size, 1));
                PUT(FOOTER(ptr), PACK(combined_size, 1));
            }
            return ptr;
        }
    }
    //Case 3: Need to move it elsewhere
    void *new_ptr = my_malloc(size);
    if (!new_ptr) return NULL;

    size_t old_paylod_size = GET_SIZE(HEADER(ptr)) - DSIZE;
    //We take the minimum of old_paylod_size and size
    size_t copy_size = (size < old_paylod_size) ? size : old_paylod_size;
    memcpy(new_ptr, ptr, copy_size);

    my_free(ptr);
    return new_ptr;
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
    
    //check prologue padding + header + footer
    if (GET(base) != 0) {
        return heap_error("alignment padding not okay",base);
    }
    
    if (GET(base+ WSIZE) != PACK(DSIZE, 1)) {
        return heap_error("bad prologue header", base + WSIZE);
    }
    if (GET(base + 2 * WSIZE) != PACK(DSIZE, 1)) {
        return heap_error("bad prologue footer", base + 2 * WSIZE);
    }
    
    void *ptr = NEXT_BLOCK(heap_start);
    int prev_free = 0;

    while(GET_SIZE(HEADER(ptr)) > 0) {

        if (!check_block(ptr)) {
            return 0;
        }

        int alloc = GET_ALLOC(HEADER(ptr));

        //Check that there is no consecutive free blocks
        if (!alloc && prev_free) {
            return heap_error("found consecutive free blocks", ptr);
        }
        
        prev_free = !alloc;

        if (NEXT_BLOCK(ptr) <= (char *)ptr) {
            return heap_error("heap traversal could not move foward", ptr);
        }

        if (!check_free_list()) return 0;

        ptr = NEXT_BLOCK(ptr);
    }

    //Check the epilogue header
    if (GET(HEADER(ptr)) != PACK(0,1)) {
        return heap_error("bad epilogue header", ptr);
    }

    return 1;
}





static void insert_free_block(void *ptr) {
    NEXT_FREE(ptr) = free_list_head;
    PREV_FREE(ptr) = NULL;

    if (free_list_head) {
        PREV_FREE(free_list_head) = ptr;
    }
    free_list_head = ptr;
}


static void remove_free_block(void *ptr) {
    void *prev = PREV_FREE(ptr);
    void *next = NEXT_FREE(ptr);

    if (prev) {
        NEXT_FREE(prev) = next;
    } else {
        free_list_head = next;
    }
    if (next) {
        PREV_FREE(next) = prev;
    }
}




static int check_free_list(void) {
    void *ptr = free_list_head;
    void *prev = NULL;

    while (ptr != NULL) {
        if (GET_ALLOC(HEADER(ptr))) {
            return heap_error("allocated block found in free list", ptr);
        }

        if (PREV_FREE(ptr) != prev) {
            return heap_error("free list previous pointer invalid", ptr);
        }

        prev = ptr;
        ptr = NEXT_FREE(ptr);
    }
    return 1;
}
















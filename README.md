# mymalloc
A general-purpose dynamic memory allocator written from scratch in C, implementing the full `malloc` / `free` / `realloc` / `calloc` interface. Designed to be a full replacement for glibc's allocator via `LD_PRELOAD`. It is fully functional with no recompilation of the target program required.


## Features
- **Explicit Free list:** Freed blocks are tracked in a doubly linked list, making allocations O(k) where k is the number of free blocks instead of O(n) over all blocks.
- **Immediate boundary-tag coalescing:** All four adjacency cases handled on every free call to eliminate heap fragmentation
- **Block splitting:** Oversized free blocks are split to minimize internal fragmentation
- **Full interface:** `malloc`, `free`, `realloc` (with in-place growth), and `calloc` (with overflow protection)
- **LD_PRELOAD compatible:** Compiles as a shared library, usable with any dynamically linked program
- **Heap consistency checker:** `heap_check()` validates heap integrity on every operation during testing
- **Full test suite:** Custom-built tests for every possible case. With a 100,000 random allocation/free operations stress test with heap integrity operation between every call
- **Valgrind:** Passes Valgrind with no errors on the entire test suite
- **Benchmarking:** Includes a custom-built benchmark program to compare throughput against glibc's `malloc` and `free`
 





## Why I Built This
I love very deep and technical subjects. Doesn't matter the field. Memory allocation is definitely an example of this and I was very curious to learn everything I could about it. I wanted to understand how allocators manage heap metadata, find free space, reuse freed blocks, and balance correctness with performance. 

I was also very interested in furthering my skills using a low-level languages like C, as well as practicing debugging low-level code, designing tests to ensure reliability and building benchmarks to measure performance. 




## Design Overview
#### A single memory block
Each block stores metadata in a header and footer using boundary tags. 

An allocated block looks like this:

```
+-----------+--------------------------+-----------+
|  HEADER   |         PAYLOAD          |  FOOTER   |
| (8 bytes) |       (user data)        | (8 bytes) |
+-----------+--------------------------+-----------+
^
Header stores: size | allocation bit    
Footer mirror header (allows for backward traversal and coalescing)

```


A free block looks like this: 
```
+-----------+------+------+----------------------+-----------+
|  HEADER   | NEXT | PREV |    (unused space)    |  FOOTER   |
+-----------+------+------+----------------------+-----------+
            ^
            NEXT and PREV are pointers to the next entries of the linked list
```

#### The entire heap
Blocks are next to each other, bounded by 3 delimiters: 
- Prologue Header
- Prologue Footer
- Epilogue Header

These blocks are always marked as allocated, acting as walls to prevent segmentation faults.

The entire heap looks like this: 

```
+------------------+-----------------+-----------------------+------------------+
|  Prologue Header | Prologue Footer | Individual blocks ... |  Epilogue Header |
+------------------+-----------------+-----------------------+------------------+
```

## Design Decisions
**Why an explicit free list over implicit?**

An implicit free list requires traversing every block (both allocated and free) to find a suitable block. On the other hand, an explicit list only visits free blocks. Under many allocation/free operations this is a significant improvement (roughly 5x faster from a specific benchmark result). This is worth it, even if it comes at the cost of 16 extra bytes per free block for the `NEXT` and `FREE` pointers. 


**Why a footer boundary tag?**

Coalescing requires knowing whether the previous block is free or not. The footer allows us to perform backward traversal through the heap in O(1) instead of having to walk the entire heap. This is worth it, even at a cost of 8 bytes per block. 


**Why `sbrk` for heap extension?**

`sbrk` extends a single contiguous heap region, which maps very cleanly onto the structure of our heap. I plan in the future to supplement our existing `sbrk` with `mmap` for large allocations (>128KB) like modern memory allocators. 


**Why a first-fit placement policy?**

First-fit is used here instead of best-fit for an increased throughput at the cost of higher fragmentation. In the future I plan to implement a segregated free list, which mimics the advantages of best-fit while improving performance. 






## Allocation Process

The allocator uses an explicit doubly linked free list and first-fit search.

The process to find a free block is therefore as follows:
1. The requested size is adjusted for alignment and metadata
2. The free list is searched for the first suitable block
3. If the block is larger than needed, we split it to avoid internal fragmentation
4. If no block is found, the heap is extended

#### Freeing and coalescing process
When a block is freed:
1. It is marked free in its header and footer
2. Adjacent blocks are checked
3. Neighboring free blocks are merged to reduce fragmentation
4. The merged block is inserted into the free list




## Building
```bash
#build static library (for tests and benchmarks)
make static

#Build shared library (for LD_PRELOAD)
make lib

#Run test suite
make test

#Run benchmarks (mymalloc vs glibc)
make bench

#Run tests under Valgrind
make valgrind
```





## Test Suite
Tests are in `tests/test_basic.c` and cover:
- **Data integrity:** Three concurrent allocations written and verified independently
- **Block reuse:** Freed blocks are reused by subsequent allocations
- **Coalescing:** Three adjacent freed blocks merge into one, able to satisfy an allocation request of their combined size without requesting memory from the OS
- **Edge cases:** Passing zero to malloc, freeing NULL, large allocations (1MB+)
- **realloc:** NULL pointer, zero size, growing in place, moving to a new location
- **calloc:** Zero initialization verified byte-by-byte, overflow protection.
- **Giga stress test:** Massive stress test of 100,000 iterations of randomized `malloc`/`free` over a 64-slot pool. Each allocation is filled with a known byte pattern and verified before freeing. `heap_check()` is called after every operation.


#### Heap Checker
To help testing and debugging, the project includes a `heap_checker` function that verifies properties such as: 
- Valid prologue and epilogue blocks
- Proper 16-byte alignment
- Matching between header and footer metadata
- Each block is of valid size
- No consecutive uncoalesced free blocks
- Integrity of the free list




```
make test
```
<img width="1109" height="289" alt="Screenshot 2026-03-22 153122" src="https://github.com/user-attachments/assets/1f1404cf-ce9b-4370-971c-d7025f9268df" />


```
make valgrind
```
<img width="1096" height="677" alt="Screenshot 2026-03-22 153150" src="https://github.com/user-attachments/assets/1865ac0c-00e9-4e47-b4dc-053be6db86b3" />




## Benchmark
1,000,000 iterations of randomized `malloc`/`free` over a 512-slot pool, randomized sizes of 16-4096 bytes, averaged over 5 runs. `memset` is called on every allocation to force real page faults. 

**Example result from the default benchmark configuration:**
<img width="955" height="781" alt="Screenshot 2026-03-22 153729" src="https://github.com/user-attachments/assets/f26aafe8-b593-4dc0-9b1f-d16db8a47d56" />

mymalloc achieved 158.77% of glibc throughput on this benchmark.


**Important Note:** mymalloc outperforms glibc on this specific benchmark because the workload is single-threaded with a fixed access pattern that favors an explicit free list. glibc's allocator is absolutely expected to significantly outperform my own implementation in throughput and consistency under any real working conditions. Future plans are to extend the benchmark suite to better evaluate real performance. 



## LD_PRELOAD - Making it Run Real Programs!
This memory allocator compiles as a shared library and can injected into any dynamically linked program via `LD_PRELOAD`: 

You can build the shared library with: 
```bash
make lib
```
This creates the `mymalloc.so` file. 

You can then use this command to run the program 
```bash
LD_PRELOAD=./mymalloc.so <program>
```

#### Some examples
**SQLite**

We can perform real database queries on mymalloc!

<img width="657" height="269" alt="Screenshot 2026-03-22 155213" src="https://github.com/user-attachments/assets/9e5d793d-fe09-47a3-a594-41f02ba14e88" />





**Python**

We can run python scripts, and even process JSON data!

<img width="1059" height="218" alt="Screenshot 2026-03-22 155559" src="https://github.com/user-attachments/assets/f0b97454-70ac-4a6f-a887-5ab5f6a0d12e" />





## Tech Stack
- C
- GCC
- Make 
- GDB
- Valgrind


## Known Limitations and Future Improvements
- **No thread safety:** There is currently a global free list with no locking. A logical next step is to implement thread safety for even more real-world uses
- **No mmap for large allocations:** Currently all allocations go through `sbrk`. Using `mmap` for larger allocations would allow large blocks to be returned back to the OS rather than be held in the heap.
- **No segregated free list:** Currently a single free list is scanned linearly. Using multiple free lists for different block sizes would greatly reduce search time for cases with high fragmentation.
- **Limited benchmarking:** Currently the benchmark tests only a specific type of use case. More types of cases should be explored to get a more realistic picture of the performance and limitations of this implementation.



## What I learned
This project taught me a whole lot about many more things than I expected. My biggest takeaway is definitely the value of and the importance of rigorous testing, benchmarking, and debugging. It also genuinely increased my appreciation of low-level systems programming. Even with the countless hours of debugging, I still had a lot of fun!



## Most annoying bug!
I'm making it a section because I spent 4 hours reading textbooks, documentation, blogs, and asking AI (who had no idea btw) to finally figure out that the thing that was breaking my entire program were the `printf()` statements I placed to help debug!
That's right, print statements can break memory allocators! 

Reason (if other people are having this issue): 
When you call `printf`, it does its own call of `malloc`. The real glibc `malloc` and not yours. That makes it so that the location of the program break moves independently of you having called `sbrk()`. Therefore, any heap traversal algorithm or heap integrity checker will not even function correctly! Very obvious in retrospect but hard to think about on the spot. 



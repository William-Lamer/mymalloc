A general purpose dynamic memory allocator written from scratch in C, utilizing an explicit doubly linked free list, boundary tag coalescing, block splitting, and a heap checker. I built it to explore low-level systems programming concepts. 


## Features
- Custom implementationsof `malloc`, `free`, and `calloc`
- Explicit doubly linked free list
- Boundary-tag coalescing
- Block splitting to reduce internal fragmentation
- 16-byte allignment (works on 32bit and 64bit systems)
- Heap consistency checker for testing and debugging purposes
- Benchmark tests comparing throughput against glibc `malloc`
- Passes Valgrind with no errors on a set of custom tests



## Why I Built This
I love very deep and technical subjects. Doesn't matter the field. Memory allocation is definitly an example of this and I was very curious to learn everything i could about it. I wanted to understand how allocators manage heap metadata, find free space, reuse freed blocks, and balance correctness with performance. 

I was also very interested in furthering my skills using a low level languages like C, as well as practicing debugging low-level code, designing tests to ensure reliability and building benchmarks to measure performance. 


## Design Overview
#### A single memory block
Each block stores metadata in a header and footer using boundary tags. 

An allocated block looks like this:

`[ header | payload ... | footer ]`

A free block looks like this: 

`[ header | next_free | prev_free | unused space ...| footer ]`

Header and footers store: 
 - The total size of the block
 - An allocation bit `0|1`

This allows the allocator to: 
- Traverse blocks in order
- Detect if neighboring blocks are free
- Coalesce adjacent free blocks in constant time

#### The entire heap
Blocks are next to each other, bounded by 3 delimiters: 
- Prologue Header
- Prologue Footer
- Epilogue Header

The entire heap looks like this: 

`[prologue header | prologue footer | block #1 | block #2 | block #i ... | Epilogue Header]`


#### Allocation Strategy

The allocator uses an explicit doubly linked free list and first-fit search.

The process to find a free block is therefore as follows:
1. The requested size is adjusted for alignment and metadata
2. The free list is searched for the first suitable block
3. If the block is larger then needed, we split it to avoid internal fragmentation
4. if no block is found, the heap is extended

###### Freeing and coalescing 
When a block is freed:
1. It is marked free in its header and footer
2. Adjacent blocks are checked
3. Neighbouring free blocks are merge to avoid reduce fragmentation
4. The merge block is inserted into the free list

#### Heap Checker
To help testing and debugging, the project includes a `heap_checker` function that verifies properties such as: 
- Valid prologue and epilogue blocks
- Proper 16-byte alignment
- Matching between header and footer metadata
- Each block is of valid size
- No consecutive uncoalesced free blocks
- Integrity of the free-list

This was especially useful when debugging, and is necessary for proper testing. 


#### Testing
The allocator was tested using:
- Custom correctness tests for each possible scenario
- Stress test of 10,000 random allocation/free calls
- Valgrind to catch invalid memory accesses and leaks

You can test using `make test` and `make valgrind`

#### Benchmark
The project includes a benchmark that compares allocator throughput against glibc `malloc`. The default settings compare them over 5 runs utilizing 1,000,000 different random allocate/free calls. It is fine tunable for the number of iterations, the minimum memory size, maximum mememory, number of concurrent pointers used and the number of times the benchmark is ran. 

Example result from one benchmark configuration:
- Custom allocator: ~31.97M ops/sec average
- glibc `malloc`: ~18.74M ops/sec average

These results are workload-dependent and reflect performance in these specific scenarios. glibc `malloc` is absolutely superior in throughput and consistency (as expected) over a far wider range of cases. 


## Instructions

##### how to install

##### Build and run

###### Run tests
```
make test
```

###### Run benchmark
```
make benchmark
```

###### Run Valgrind
```
make valgrind
```


### Tech Stack
- C
- GCC
- Make 
- GDB
- Valgrind


## Future Improvments
There are many ways to improve this project. The potential next steps are: 
- Using `mmap` for larger allocation, like modern memory allocators
- Moving to a segrageted list for even faster performance
- Extending the benchmark suite for a wider range of cases



## What I learned
This project taught me a whole lot about many more thins than I expected. My biggest takeaway is definitly the value of and the importance of rigourous testing, benchmarking, and debugging. It also genuinly increased my appreciation of low-level systems programming. Even with the countless hours of debugging, I still had a lot of fun!



## Most annoying bug!
I'm making it a section because I spent 4 hours reading textbooks, documentation, blogs and asking AI (who had no idea) to finally figure out that the thing that was breaking my entire program were the `printf()` statements I placed to help debug!
That's right, print statements can break memory allocators! 

Reason (if other people are having this issue): 
When you call `printf`, it does its own call of `malloc`. The real glibc `malloc` and not yours. That makes it so that the location of the program break moves independantly of you having called `sbrk()`. Therefore any heap traversal algorithm or heap integrity checker will not even function correctly! Very obvious in retrospect but hard to think about on the spot. 































































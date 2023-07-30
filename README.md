# Heap Allocator in C

This repository contains implementation of a basic heap allocator in C. It is organized around two main files:

1. **implicit.c**: Implementation of the implicit heap allocator.
2. **explicit.c**: Implementation of the explicit heap allocator.

## Description
The implicit heap allocator builds out a very simple heap allocator with little optimizaton. 

The explicit heap allocator implementation utilizes an explicit free list to manage memory. Headers contain the payload of each memory block, along with a list of all free blocks. This allows for efficient storage of memory, as well as quick access to free memory when required.

## Functions

Here's a brief overview of some of the main functions in this repository:

- `myinit()`: Initializes the heap to the proper form.
- `mymalloc()`: Allocates a block on the heap that can hold a certain amount of memory.
- `myfree()`: Frees the block that stores the memory specified by a given pointer.
- `myrealloc()`: Changes the size of the block pointed to by a given pointer to a new size.
- `validate_heap()`: Checks the integrity of the heap segment.

## Usage

After cloning this repository, compile the program using a C compiler like `gcc` and run the program:

```bash
gcc -o allocator explicit.c

./allocator
```

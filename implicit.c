/* implicit.c
 * This program implements an implicit heap allocator for managing heap memory. 
 * It maintains information about memory blocks using 'header' structures which store payload sizes.
 * Since all addresses and payload values must be multiples of 8, the three least significant bits (LSB) of the payload are used to store the allocation status of each memory block. 
 */
#include "allocator.h"
#include "debug_break.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* Each block in the heap begins with a header. The header stores the payload size of the block. */
typedef struct header{
    unsigned long payload_size;
} header;

header* segment_start; // Pointer to the start of the memory segment
size_t segment_size; // Total size of the memory segment
void* heap_end; // Pointer to the end of the memory segment

const unsigned long FREE_MASK = 7; // Mask to extract the allocation status from the payload size
const unsigned long PAYLOAD_MASK = ~7; // Mask to extract the payload size from the header

/* roundup
------------
 Given a size and a multiple, this function returns the smallest multiple of 'mult' 
 that is greater than or equal to 'sz'. It uses bitwise logic to efficiently compute the 
 rounded value.

 @param sz: the size to round up
 @param mult: the multiple to round up to
 @return: the rounded size
*/
size_t roundup(size_t sz, size_t mult) {
    return (sz + mult - 1) & ~(mult - 1);
}

/* myinit
-----------
 This function initializes the heap segment. It does this by setting up the heap_start, 
 heap_size, and heap_end variables. The heap initially starts with a single free block. 
 If the heap size is too small to be useful, the function returns false.

 @param heap_start: pointer to the start of the heap
 @param heap_size: the size of the heap
 @return: true if successful, false otherwise
*/
bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < ALIGNMENT * 2) { //check for min heap_size
        return false;
    }

    segment_start = (header*)heap_start;
    segment_size = heap_size;
    heap_end = (char*)heap_start + heap_size;
    (*segment_start).payload_size = segment_size - ALIGNMENT;
    return true;
}

/* mymalloc
-------------
 This function attempts to allocate a block of memory on the heap of size 'requested_size'. 
 It iteratively checks each block in the heap until it finds a free block that is big enough to 
 satisfy the request. In case of the last block, it attempts to split it if it's possible. 
 If allocation fails, it returns NULL.

 @param requested_size: the requested size for the new block
 @return: a pointer to the allocated block, or NULL if allocation failed
*/
void *mymalloc(size_t requested_size) {
     if (requested_size == 0) {
        return NULL;
    }
     
    size_t request = roundup(requested_size, ALIGNMENT);
    char* index = (char*)segment_start;
    header* block = segment_start;

    while((void*)index != heap_end) {
        unsigned long payload = (*block).payload_size;
        bool free = !(payload & FREE_MASK);
        unsigned long payload_value = payload & PAYLOAD_MASK;
        
        if (index + payload_value + ALIGNMENT == heap_end) { //last block case

            if (!free || request > payload_value) { //last block not free
                return NULL;
            } 

            if (free && payload_value >= request + (ALIGNMENT * 2)) { //add another header
                (*block).payload_size = request + 1;
                header* new = (header*)(index + request + ALIGNMENT); 
                (*new).payload_size = payload_value - request - ALIGNMENT;
                return (void*)(index + ALIGNMENT);
            }

            (*block).payload_size = request + 1; //just fits in last block
            return (void*)(index + ALIGNMENT);
            
        } else { //not on last block, check free and fits

            if (free && payload_value >= request) { //curr block fit
                (*block).payload_size += 1;
                return (void*)(index + ALIGNMENT);
            } 

            index += payload_value + ALIGNMENT; //move onto next block
            block = (header*)index;
            
        }      
    }  
    return NULL;
}

/* myfree
-----------
 This function attempts to free the block pointed to by 'ptr'. It does this by 
 updating the payload size of the block's header to indicate that it is free. 
 If 'ptr' is NULL, the function does nothing.

 @param ptr: the pointer to the block to be freed
 @return: void
*/
void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }
    header* block = (header*)((char*)ptr - ALIGNMENT);
    (*block).payload_size -= 1;
}

/* myrealloc
--------------
 This function changes the size of the block pointed to by 'old_ptr' to 'new_size'. 
 If 'old_ptr' is NULL, it behaves like mymalloc. If 'new_size' is 0, it behaves like 
 myfree. It also copies the contents from the old block to the new one, and then frees 
 the old block.

 @param old_ptr: the pointer to the block to be reallocated
 @param new_size: the new size for the block
 @return: a pointer to the newly allocated block, or NULL if reallocation failed
*/
void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) { 
        return mymalloc(new_size);
    }

    if (old_ptr != NULL && new_size == 0) {
        myfree(old_ptr);
        return NULL; 
    } 

    void* new_ptr = mymalloc(new_size);
    void* check = memcpy(new_ptr, old_ptr, new_size);
    assert(check != NULL);
    myfree(old_ptr);
    return new_ptr;
}

/* validate_heap
-----------------
 This function checks the validity of the heap. It iteratively checks each block in the 
 heap to ensure that it fits within the heap segment. It returns false if a block is 
 found that goes outside the heap segment.

 @return: true if the heap is valid, false otherwise
*/
bool validate_heap() {
    char* index = (char*)segment_start;
    header* block = segment_start;
  
    while ((void*)index != heap_end) {
        unsigned long payload = (*block).payload_size;
        unsigned long payload_val = payload & PAYLOAD_MASK;

        if (index > (char*)heap_end) {
            return false; //block goes outside heap segment
        }   

        index += payload_val + ALIGNMENT; //move onto next block
        block = (header*)index;            
    }       

    return true;
}

/* dump_heap
--------------
 This function prints the contents of the heap for debugging purposes. For each block, 
 it prints the block address, the payload size (including the status bits), and the 
 payload size (excluding the status bits).

 @return: void
*/
void dump_heap() {
    char* index = (char*)segment_start;
    header* block = segment_start;

    while ((void*)index != heap_end) {
        unsigned long payload = (*block).payload_size;
        unsigned long payload_val = payload & PAYLOAD_MASK;

        printf("address: %p", (void*)block);
        printf(" header: %lu", payload);
        printf(" payload: %lu\n", payload_val);

        index += payload_val + ALIGNMENT; //move onto next block
        block = (header*)index;            
    }       
}

    
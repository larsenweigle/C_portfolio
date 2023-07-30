/* explicit.c
---------------
 Explicit.c organizes the heap using header that contain the payload of each memory block, 
 along with a list of all free blocks. These properties make explicit.c more efficient at 
 storing memory, as well as allowing the user to quickly access free memory when it is needed. 
 */
#include "allocator.h"
#include "debug_break.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// struct used to store the information of each header. 
typedef struct header{
    unsigned long payload;
    void* prev;
    void* next;
} header;

header* segment_start;
header* freelist_start;
size_t segment_size;
void* heap_end;

const unsigned long FREE_MASK = 7;
const unsigned long PAYLOAD_MASK = ~7;

/* myinit
---------------
 Initializes the heap memory to be managed by the allocator. The heap memory starts
 with a single free block represented by a header that contains the size of the heap
 and two NULL pointers indicating that it is not linked with other free blocks. 

 @param heap_start: pointer to the start of the heap memory to be managed
 @param heap_size: the size in bytes of the heap memory to be managed
 @return: true if the heap was initialized successfully, false otherwise
*/
bool myinit(void *heap_start, size_t heap_size) {
    if (heap_size < ALIGNMENT * 3) { //check for min heap_size
        return false;
    }
    
    segment_size = heap_size;
    heap_end = (char*)heap_start + heap_size;
    segment_start = (header*)heap_start;
    (*segment_start).payload = segment_size - ALIGNMENT;
    (*segment_start).prev = NULL;
    (*segment_start).next = NULL;
    freelist_start = segment_start;

    return true;
}

/* get_payload
---------------
 Retrieves the payload size from a header block. The payload size is the amount 
 of usable memory in a block, excluding the size of the header itself. It uses 
 bitwise operations to remove the last three bits that indicate the free/used status.

 @param block: pointer to the header block
 @return: the size of the payload in the block
*/
unsigned long get_payload(header* block) {
    return (*block).payload & PAYLOAD_MASK;
}

/* check_free
---------------
 Determines whether a given block is free or used. This function extracts the value 
 of the three least significant bits of the payload. A value of zero means the block 
 is free, while a value of one means the block is used.

 @param block: pointer to the header block
 @return: true if the block is free, false otherwise
*/
bool check_free(header* block) {
    if (block == NULL) {
        return false;
    }
    return !((*block).payload & FREE_MASK);
}

/* roundup
------------
 Rounds up the given size to the nearest multiple of the alignment size.

 @param sz: the original size
 @param mult: the alignment size
 @return: the size rounded up to the nearest multiple of the alignment size
*/
size_t roundup(size_t sz, size_t mult) {
    size_t corrected = (sz + mult - 1) & ~(mult - 1);
    if (corrected < 16) {
        return 16;
    }
    return corrected;
}

/* search_freelist
--------------------
 Searches the list of free blocks and returns the first block that is large enough 
 to accommodate the requested size. If no suitable block is found, it returns NULL.

 @param request: the requested size for the block
 @return: pointer to the first free block large enough to accommodate the request, or NULL if no such block is found
*/
header* search_freelist(size_t request) {
    header* curr = freelist_start;

    while(curr != NULL) {
        bool free = check_free(curr);
        if (free && get_payload(curr) >= request) {
            return curr;
        }
        curr = (header*)(*curr).next;
    }

    return NULL;
}

/* remove_freelist
--------------------
 Removes a block from the list of free blocks. This is typically used when a free 
 block is allocated and is no longer available for use.

 @param new: pointer to the block to be removed from the free list
*/
void remove_freelist(header* new) {
    header curr = *new;

    if (curr.prev == NULL) { //first element in linked list

        if (curr.next == NULL) { //only elememnt case
            freelist_start = NULL;
            return;
        }
 
        header* new_front = (header*)curr.next;
        freelist_start = new_front;
        (*new_front).prev = NULL;
        return;
    }
    
    header* next_header = (header*)(curr.next);
    header* prev_header = (header*)(curr.prev);

    if (next_header != NULL) {
        (*next_header).prev = (void*)prev_header;
    }
    if (prev_header != NULL) {
        (*prev_header).next = (void*)next_header;
    }
}

/* get_next_block
-------------------
 Retrieves the block that comes after a given block in memory.

 @param block: pointer to the block
 @return: pointer to the next block in memory, or NULL if the given block is the last one in memory
*/
header* get_next_block(header* block) {
    unsigned long payload_val = get_payload(block);
    char* next_location = (char*)block + payload_val + ALIGNMENT;
    if ((void*)next_location == heap_end) {
        return NULL;
    } 
    return (header*)next_location;
}

/* coalesce
-------------
 Combines a block with the next block in memory if the next block is free, effectively 
 creating a larger free block. This helps in reducing fragmentation and making larger 
 chunks of memory available for allocation.

 @param block: pointer to the block to be coalesced with the next block
*/
void coalesce(header* block) {
    header* next_block = get_next_block(block);
    bool free = check_free(next_block);

    if (next_block == NULL || !free) {
        return;
    }
    
    unsigned long next_payload_val = get_payload(next_block);
    unsigned long added_space = next_payload_val + ALIGNMENT;
    remove_freelist(next_block);
    (*block).payload += added_space;
    
}

/* add_freelist
-------------------
 Adds a block to the free list. This typically happens when a block is freed or when 
 a large block is split into two smaller blocks.

 @param new: pointer to the block to be added to the free list
*/
void add_freelist(header* new) {
    if (freelist_start == NULL) {
        freelist_start = new;
        (*new).prev = NULL;
        (*new).next = NULL;
        return;
    }

    (*freelist_start).prev = (void*)new;
    (*new).next = (void*)freelist_start;
    (*new).prev = NULL;
    freelist_start = new;
}

/* add_block
--------------
 Splits a block into two: one of the requested size and the other containing the remaining space. 

 @param block: pointer to the block to be split
 @param request: the requested size for the new block
*/
void add_block(header* block, size_t request) {
    bool free = check_free(block);
    char* location = (char*)block;
    unsigned long payload_val = get_payload(block);   
    (*block).payload = request + 1;
    header* new = (header*)(location + request + ALIGNMENT);
    (*new).payload = payload_val - request - ALIGNMENT;
    if (free) {
        remove_freelist(block);
    }
    add_freelist(new);
    coalesce(new);
}

/* coalesce_multiple_blocks
-----------------------------
 Continuously merges a block with its subsequent free blocks until it has enough space 
 to accommodate the specified size.

 @param block: pointer to the block to be coalesced with subsequent free blocks
 @param req: the requested size that the block should be able to accommodate after coalescing
*/
void coalesce_multiple_blocks(header* block, unsigned long req) {
    header* next_block = get_next_block(block); //coalesce right block while it is free
    while (check_free(next_block) && get_payload(block) < req) {
        coalesce(block);
        next_block = get_next_block(block);
    }
}

/* mymalloc
-------------
 Allocates a block of memory of the specified size from the heap. It does this by searching 
 the free block list for a suitable block. If a suitable block is found, it is removed from 
 the free list and returned to the caller.

 @param requested_size: the size in bytes of the block to be allocated
 @return: a pointer to the allocated block, or NULL if allocation failed
*/
void *mymalloc(size_t requested_size) {
    size_t request = roundup(requested_size, ALIGNMENT);  
    header* free_location = search_freelist(request); 

    if (free_location == NULL) { //no available blocks 
        return NULL;
    }
    if (request > MAX_REQUEST_SIZE) {
        return NULL;
    }

    unsigned long payload_val = get_payload(free_location);
    char* location = (char*)free_location;

    if (location + payload_val + ALIGNMENT == heap_end) {//last block and add new header special case

        if (payload_val >= request + (ALIGNMENT * 3)) {
            add_block(free_location, request);
            return (void*)(location + ALIGNMENT);
        }
        
    }

    (*free_location).payload += 1; //update the free block
    remove_freelist(free_location);
    return (void*)(location + ALIGNMENT);     
}

/* myfree
-----------
 Frees a block of memory, making it available for future allocations. The block is added 
 back to the free list and coalesced with any adjacent free blocks.

 @param ptr: pointer to the block to be freed
*/
void myfree(void *ptr) {
    if (ptr == NULL) {
        return;
    }

    header* block = (header*)((char*)ptr - ALIGNMENT);
    (*block).payload -= 1;
    add_freelist(block);
    coalesce(block);   
}

/* myrealloc
--------------
 Resizes an allocated block to a new size. If the block is large enough to accommodate the 
 new size, it is split into two: one of the new size and the other containing the remaining 
 space. If the block is not large enough, a new block of the requested size is allocated, 
 the contents of the old block are copied to the new block, and the old block is freed.

 @param old_ptr: the pointer to the block to be reallocated
 @param new_size: the new size for the block
 @return: a pointer to the newly allocated block, or NULL if reallocation failed
*/
void *myrealloc(void *old_ptr, size_t new_size) {
    if (old_ptr == NULL) {
        return mymalloc(new_size);
    }

    unsigned long request = roundup(new_size, ALIGNMENT);
    header* old_header = (header*)((char*)old_ptr - ALIGNMENT);
    unsigned long old_size = get_payload(old_header);

    if (new_size == 0) { //realloc(0) case
        myfree(old_ptr);
        return NULL; 
    }

    coalesce_multiple_blocks(old_header, request); //coalsce until enough space or right block occupied

    if (get_payload(old_header) >= request) { //in place realloc
        void* block_end = (void*)(get_payload(old_header) + (char*)old_ptr);

        if (block_end == heap_end && request + (ALIGNMENT * 3) < get_payload(old_header)) {
            header* new = (header*)((char*)old_ptr + request); 
            (*new).payload = get_payload(old_header) - request - ALIGNMENT;
            (*old_header).payload = request + 1;
            add_freelist(new);
        } //last block on the heap, prevents heap exhuastion 

        if (block_end != heap_end && old_size >= request + (ALIGNMENT * 3)) {  
            add_block(old_header, request);
        }
        
        return old_ptr;
    }
    
    void* new_ptr = mymalloc(new_size);
    void* check = memcpy(new_ptr, old_ptr, old_size);
    assert(check != NULL);
    myfree(old_ptr);
    return new_ptr;
}

/* validate_heap
------------------
 Validates the state of the heap. It checks whether the blocks are correctly aligned, 
 whether the total size of the blocks matches the size of the heap, whether there are 
 any overlapping blocks, and whether the free list correctly contains all the free blocks.

 @return: true if the heap is valid, false otherwise
*/
bool validate_heap() {
    char* index = (char*)segment_start;
    header* block = segment_start;
    unsigned long total_heap_used = 0;
    while ((void*)index != heap_end) {
        unsigned long payload_val = get_payload(block);
        if (index > (char*)heap_end) {
            return false; //block goes outside heap segment
        }
        if ((payload_val % ALIGNMENT) != 0) {
            return false;
        }
 
        total_heap_used += payload_val + ALIGNMENT;
        index += payload_val + ALIGNMENT;
        block = (header*)index;
    }

    if (total_heap_used != segment_size) {
        return false;
    }

    header* curr = freelist_start;
    while (curr != NULL) {
        bool free = check_free(curr);
        if (!free || (get_payload(curr) % ALIGNMENT) != 0) {
            return false;
        }
        curr = (header*)(*curr).next;
    }

    return true;
}

/* dump_heap
--------------
 Prints the current state of the heap. It prints the payload size and the free/used status 
 of each block, as well as the total size of the heap and the total amount of free space.
*/
void dump_heap() {
    char* index = (char*)segment_start;
    header *block = segment_start;

    while((void*)index != heap_end) {
        unsigned long payload_val = get_payload(block);
        bool free = check_free(block);

        printf("Block at %p: payload=%lu, free=%s\n", index, payload_val, free ? "true" : "false");
        
        index += payload_val + ALIGNMENT;
        block = (header*)index;
    }
}
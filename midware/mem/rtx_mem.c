#include "rtx_mem.h"
#include "cfg.h"


//  Memory Pool Header structure
typedef struct {
  uint32_t size;                // Memory Pool size
  uint32_t used;                // Used Memory
} mem_head_t;

//  Memory Block Header structure
typedef struct mem_block_s {
  struct mem_block_s *next;     // Next Memory Block in list
  uint32_t            size;     // Block Info or max used Memory (in last block)
} mem_block_t;

#define HEAD_SIZE               sizeof(mem_head_t)
#define BLOCK_SIZE              sizeof(mem_block_t)



static inline mem_head_t *head_ptr (void *mem)
{
    return ((mem_head_t *)mem);
}
static inline mem_block_t *block_ptr (void *mem, uint32_t offset)
{
    return (mem_block_t *)((uint32_t)mem + offset);
}




/// Initialize Memory Pool with variable block size.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  size            size of a memory pool in bytes.
/// \return      0: success,    -1: failure.
int rtx_mem_init(void *mem, uint32_t size)
{
    mem_head_t  *head;
    mem_block_t *block;

    // Check parameters
    //lint -e{923} "cast from pointer to unsigned int" [MISRA Note 7]
    if ((mem == NULL) || (((uint32_t)mem & 7U) != 0U) || ((size & 7U) != 0U) ||
        (size < (HEAD_SIZE + (2U*BLOCK_SIZE)))) {
        return -1;
    }

    // Initialize memory pool header
    head = head_ptr(mem);
    head->size = size;
    head->used = HEAD_SIZE + BLOCK_SIZE;

    // Initialize first and last block header
    block = block_ptr(mem, HEAD_SIZE);
    block->next = block_ptr(mem, size - BLOCK_SIZE);
    block->next->next = NULL;
    block->next->size = HEAD_SIZE + BLOCK_SIZE;
    block->size = 0U;

  return 0;
}

/// Allocate a memory block from a Memory Pool.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  size            size of a memory block in bytes.
/// \param[in]  type            memory block type: 0 - generic, 1 - control block
/// \return allocated memory block or NULL in case of no memory is available.
void *rtx_mem_alloc(void *mem, uint32_t size)
{
    int t=1;
    mem_head_t  *head;
    mem_block_t *ptr,*last;
    mem_block_t *p,*p_new;
    uint32_t     block_size;
    uint32_t     hole_size;

    // Check parameters
    if ((mem == NULL) || (size == 0U)) {
        return NULL;
    }

    // Add block header to size
    block_size = size + BLOCK_SIZE;
    // Make sure that block is 8-byte aligned
    block_size = (block_size + 7U) & ~((uint32_t)7U);

    // Search for hole big enough
    p = block_ptr(mem, HEAD_SIZE);      //p point to the first block
    for (;;) {
        hole_size  = (uint32_t)p->next - (uint32_t)p;
        hole_size -= p->size;
        if (hole_size >= block_size) { // Hole found
            break;
        }
        
        p = p->next;
        if (p->next == NULL) {  // Failed (end of list)
            return NULL;
        }
    }

    // Update used memory
    head = head_ptr(mem);
    head->used += block_size;

    // Update max used memory
    last = block_ptr(mem, head->size - BLOCK_SIZE);
    if (last->size < head->used) {
        last->size = head->used;
    }

    // Allocate block
    if (p->size == 0U) {
        // No block allocated, set info of first element
        p->size = block_size;
        ptr = block_ptr(p, BLOCK_SIZE);
    } else {
        // Insert new element into the list
        p_new = block_ptr(p, p->size);
        p_new->next = p->next;
        p_new->size = block_size;
        p->next = p_new;
        ptr = block_ptr(p_new, BLOCK_SIZE);
    }
    
    return ptr;
}

/// Return an allocated memory block back to a Memory Pool.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  block           memory block to be returned to the memory pool.
/// \return      0: success,    -1: failure.
int rtx_mem_free (void *mem, void *block)
{
  const mem_block_t *ptr;
        mem_block_t *p, *p_prev;

    // Check parameters
    if ((mem == NULL) || (block == NULL)) {
        return -1;
    }

    // Memory block header
    ptr = block_ptr(block, 0U);
    ptr--;

    // Search for block header
    p_prev = NULL;
    p = block_ptr(mem, HEAD_SIZE);
    while (p != ptr) {
        p_prev = p;
        p = p->next;
        if (p == NULL) {
            // Not found
            return -1;
        }
    }

    // Update used memory
    (head_ptr(mem))->used -= p->size;

    // Free block
    if (p_prev == NULL) {
        // Release first block, only set info to 0
        p->size = 0U;
    } else {
        // Discard block from chained list
        p_prev->next = p->next;
    }

    return 0;
}



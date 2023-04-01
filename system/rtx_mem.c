#include "rtx_mem.h"
#include "cfg.h"


#define MB_INFO_LEN_MASK        0xFFFFFFFCU     // Length mask
#define MB_INFO_TYPE_MASK       0x00000003U     // Type mask

static inline rtx_mem_head_t *MemHeadPtr (void *mem)
{
    return ((rtx_mem_head_t *)mem);
}
static inline rtx_mem_block_t *MemBlockPtr (void *mem, uint32_t offset)
{
  uint32_t     addr;
  rtx_mem_block_t *ptr;

  //lint --e{923} --e{9078} "cast between pointer and unsigned int" [MISRA Note 8]
  addr = (uint32_t)mem + offset;
  ptr  = (rtx_mem_block_t *)addr;

  return ptr;
}




/// Initialize Memory Pool with variable block size.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  size            size of a memory pool in bytes.
/// \return 1 - success, 0 - failure.
uint32_t rtx_mem_init(void *mem, uint32_t size)
{
  rtx_mem_head_t  *head;
  rtx_mem_block_t *ptr;

  // Check parameters
  //lint -e{923} "cast from pointer to unsigned int" [MISRA Note 7]
  if ((mem == NULL) || (((uint32_t)mem & 7U) != 0U) || ((size & 7U) != 0U) ||
      (size < (sizeof(rtx_mem_head_t) + (2U*sizeof(rtx_mem_block_t))))) {
    //EvrRtxMemoryInit(mem, size, 0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  // Initialize memory pool header
  head = MemHeadPtr(mem);
  head->size = size;
  head->used = sizeof(rtx_mem_head_t) + sizeof(rtx_mem_block_t);

  // Initialize first and last block header
  ptr = MemBlockPtr(mem, sizeof(rtx_mem_head_t));
  ptr->next = MemBlockPtr(mem, size - sizeof(rtx_mem_block_t));
  ptr->next->next = NULL;
  ptr->next->info = sizeof(rtx_mem_head_t) + sizeof(rtx_mem_block_t);
  ptr->info = 0U;

  //EvrRtxMemoryInit(mem, size, 1U);

  return 1U;
}

/// Allocate a memory block from a Memory Pool.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  size            size of a memory block in bytes.
/// \param[in]  type            memory block type: 0 - generic, 1 - control block
/// \return allocated memory block or NULL in case of no memory is available.
void *rtx_mem_alloc(void *mem, uint32_t size, uint32_t type)
{
  rtx_mem_block_t *ptr;
  rtx_mem_block_t *p, *p_new;
  uint32_t     block_size;
  uint32_t     hole_size;

  // Check parameters
  if ((mem == NULL) || (size == 0U) || ((type & ~MB_INFO_TYPE_MASK) != 0U)) {
    //EvrRtxMemoryAlloc(mem, size, type, NULL);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return NULL;
  }

  // Add block header to size
  block_size = size + sizeof(rtx_mem_block_t);
  // Make sure that block is 8-byte aligned
  block_size = (block_size + 7U) & ~((uint32_t)7U);

  // Search for hole big enough
  p = MemBlockPtr(mem, sizeof(rtx_mem_head_t));
  for (;;) {
    //lint -e{923} -e{9078} "cast from pointer to unsigned int"
    hole_size  = (uint32_t)p->next - (uint32_t)p;
    hole_size -= p->info & MB_INFO_LEN_MASK;
    if (hole_size >= block_size) {
      // Hole found
      break;
    }
    p = p->next;
    if (p->next == NULL) {
      // Failed (end of list)
      //EvrRtxMemoryAlloc(mem, size, type, NULL);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return NULL;
    }
  }

  // Update used memory
  (MemHeadPtr(mem))->used += block_size;

  // Update max used memory
  p_new = MemBlockPtr(mem, (MemHeadPtr(mem))->size - sizeof(rtx_mem_block_t));
  if (p_new->info < (MemHeadPtr(mem))->used) {
    p_new->info = (MemHeadPtr(mem))->used;
  }

  // Allocate block
  if (p->info == 0U) {
    // No block allocated, set info of first element
    p->info = block_size | type;
    ptr = MemBlockPtr(p, sizeof(rtx_mem_block_t));
  } else {
    // Insert new element into the list
    p_new = MemBlockPtr(p, p->info & MB_INFO_LEN_MASK);
    p_new->next = p->next;
    p_new->info = block_size | type;
    p->next = p_new;
    ptr = MemBlockPtr(p_new, sizeof(rtx_mem_block_t));
  }

  //EvrRtxMemoryAlloc(mem, size, type, ptr);

  return ptr;
}

/// Return an allocated memory block back to a Memory Pool.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  block           memory block to be returned to the memory pool.
/// \return 1 - success, 0 - failure.
uint32_t rtx_mem_free (void *mem, void *block) {
  const rtx_mem_block_t *ptr;
        rtx_mem_block_t *p, *p_prev;

  // Check parameters
  if ((mem == NULL) || (block == NULL)) {
    //EvrRtxMemoryFree(mem, block, 0U);
    //lint -e{904} "Return statement before end of function" [MISRA Note 1]
    return 0U;
  }

  // Memory block header
  ptr = MemBlockPtr(block, 0U);
  ptr--;

  // Search for block header
  p_prev = NULL;
  p = MemBlockPtr(mem, sizeof(rtx_mem_head_t));
  while (p != ptr) {
    p_prev = p;
    p = p->next;
    if (p == NULL) {
      // Not found
      //EvrRtxMemoryFree(mem, block, 0U);
      //lint -e{904} "Return statement before end of function" [MISRA Note 1]
      return 0U;
    }
  }

  // Update used memory
  (MemHeadPtr(mem))->used -= p->info & MB_INFO_LEN_MASK;

  // Free block
  if (p_prev == NULL) {
    // Release first block, only set info to 0
    p->info = 0U;
  } else {
    // Discard block from chained list
    p_prev->next = p->next;
  }

  //EvrRtxMemoryFree(mem, block, 1U);

  return 1U;
}



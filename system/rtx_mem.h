#ifndef __RTX_MEM_Hx__
#define __RTX_MEM_Hx__

#include <stddef.h>
#include <stdint.h>


//  Memory Pool Header structure
typedef struct {
  uint32_t size;                // Memory Pool size
  uint32_t used;                // Used Memory
} rtx_mem_head_t;

//  Memory Block Header structure
typedef struct mem_block_s {
  struct mem_block_s *next;     // Next Memory Block in list
  uint32_t            info;     // Block Info or max used Memory (in last block)
} rtx_mem_block_t;


/// Initialize Memory Pool with variable block size.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  size            size of a memory pool in bytes.
/// \return 1 - success, 0 - failure.
uint32_t rtx_mem_init (void *mem, uint32_t size);


/// Allocate a memory block from a Memory Pool.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  size            size of a memory block in bytes.
/// \param[in]  type            memory block type: 0 - generic, 1 - control block
/// \return allocated memory block or NULL in case of no memory is available.
void *rtx_mem_alloc (void *mem, uint32_t size, uint32_t type);



/// Return an allocated memory block back to a Memory Pool.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  block           memory block to be returned to the memory pool.
/// \return 1 - success, 0 - failure.
uint32_t rtx_mem_free (void *mem, void *block);
    
#endif
    
    
    

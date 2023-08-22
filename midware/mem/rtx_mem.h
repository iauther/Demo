#ifndef __RTX_MEM_Hx__
#define __RTX_MEM_Hx__

#include <stddef.h>
#include <stdint.h>


/// Initialize Memory Pool with variable block size.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  size            size of a memory pool in bytes.
/// \return 1 - success, 0 - failure.
int rtx_mem_init (void *mem, uint32_t size);


/// Allocate a memory block from a Memory Pool.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  size            size of a memory block in bytes.
/// \return allocated memory block or NULL in case of no memory is available.
void *rtx_mem_alloc (void *mem, uint32_t size);



/// Return an allocated memory block back to a Memory Pool.
/// \param[in]  mem             pointer to memory pool.
/// \param[in]  block           memory block to be returned to the memory pool.
/// \return 1 - success, 0 - failure.
int rtx_mem_free (void *mem, void *block);
    
#endif
    
    
    

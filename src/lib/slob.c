#include "common/types.h"
#include "common/errors.h"
#include "common/fsl_string.h"
#include "common/fsl_malloc.h"
#include "kernel/fsl_spinlock.h"
#include "common/dbg.h"

#include "slob.h"


/**********************************************************************
 *                     MM internal routines set                       *
 **********************************************************************/

/****************************************************************
 *  Routine:   CreateBusyBlock
 *
 *  Description:
 *      Initializes a new busy block of "size" bytes and started
 *      rom "base" address. Each busy block has a name that
 *      specified the purpose of the memory allocation.
 *
 *  Arguments:
 *      base      - base address of the busy block
 *      size      - size of the busy block
 *      name      - name that specified the busy block
 *
 *  Return value:
 *      A pointer to new created structure returned on success;
 *      Otherwise, NULL.
 ****************************************************************/
static t_busy_block * create_busy_block(uint64_t base, uint64_t size, char *name)
{
    t_busy_block *p_busy_block;
    uint32_t    n;

    p_busy_block = (t_busy_block *)fsl_os_malloc(sizeof(t_busy_block));
    if ( !p_busy_block )
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
        return NULL;
    }

    p_busy_block->base = base;
    p_busy_block->end = base + size;

    n = strlen(name);
    if (n >= MM_MAX_NAME_LEN)
        n = MM_MAX_NAME_LEN - 1;
    strncpy(p_busy_block->name, name, MM_MAX_NAME_LEN-1);
    p_busy_block->name[n] = '\0';
    p_busy_block->p_next = 0;

    return p_busy_block;
}

/****************************************************************
 *  Routine:   CreateNewBlock
 *
 *  Description:
 *      Initializes a new memory block of "size" bytes and started
 *      from "base" address.
 *
 *  Arguments:
 *      base    - base address of the memory block
 *      size    - size of the memory block
 *
 *  Return value:
 *      A pointer to new created structure returned on success;
 *      Otherwise, NULL.
 ****************************************************************/
static t_mem_block * create_new_block(uint64_t base, uint64_t size)
{
    t_mem_block *p_mem_block;

    p_mem_block = (t_mem_block *)fsl_os_malloc(sizeof(t_mem_block));
    if ( !p_mem_block )
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
        return NULL;
    }

    p_mem_block->base = base;
    p_mem_block->end = base+size;
    p_mem_block->p_next = 0;

    return p_mem_block;
}

/****************************************************************
 *  Routine:   CreateFreeBlock
 *
 *  Description:
 *      Initializes a new free block of of "size" bytes and
 *      started from "base" address.
 *
 *  Arguments:
 *      base      - base address of the free block
 *      size      - size of the free block
 *
 *  Return value:
 *      A pointer to new created structure returned on success;
 *      Otherwise, NULL.
 ****************************************************************/
static t_free_block * create_free_block(uint64_t base, uint64_t size)
{
    t_free_block *p_free_block;

    p_free_block = (t_free_block *)fsl_os_malloc(sizeof(t_free_block));
    if ( !p_free_block )
    {
        REPORT_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
        return NULL;
    }

    p_free_block->base = base;
    p_free_block->end = base + size;
    p_free_block->p_next = 0;

    return p_free_block;
}

/****************************************************************
 *  Routine:    AddFree
 *
 *  Description:
 *      Adds a new free block to the free lists. It updates each
 *      free list to include a new free block.
 *      Note, that all free block in each free list are ordered
 *      by their base address.
 *
 *  Arguments:
 *      p_MM  - pointer to the MM object
 *      base  - base address of a given free block
 *      end   - end address of a given free block
 *
 *  Return value:
 *
 *
 ****************************************************************/
static int add_free(t_MM *p_MM, uint64_t base, uint64_t end)
{
    t_free_block *p_prev_b, *p_curr_b, *p_new_b;
    uint64_t    alignment;
    uint64_t    align_base;
    int         i;

    /* Updates free lists to include  a just released block */
    for (i=0; i <= MM_MAX_ALIGNMENT; i++)
    {
        p_prev_b = p_new_b = 0;
        p_curr_b = p_MM->free_blocks[i];

        alignment = (uint64_t)(0x1 << i);
        align_base = MAKE_ALIGNED(base, alignment);

        /* Goes to the next free list if there is no block to free */
        if (align_base >= end)
            continue;

        /* Looks for a free block that should be updated */
        while ( p_curr_b )
        {
            if ( align_base <= p_curr_b->end )
            {
                if ( end > p_curr_b->end )
                {
                    t_free_block *p_next_b;
                    while ( p_curr_b->p_next && end > p_curr_b->p_next->end )
                    {
                        p_next_b = p_curr_b->p_next;
                        p_curr_b->p_next = p_curr_b->p_next->p_next;
                        fsl_os_free(p_next_b);
                    }

                    p_next_b = p_curr_b->p_next;
                    if ( !p_next_b || (p_next_b && end < p_next_b->base) )
                    {
                        p_curr_b->end = end;
                    }
                    else
                    {
                        p_curr_b->end = p_next_b->end;
                        p_curr_b->p_next = p_next_b->p_next;
                        fsl_os_free(p_next_b);
                    }
                }
                else if ( (end < p_curr_b->base) && ((end-align_base) >= alignment) )
                {
                    if ((p_new_b = create_free_block(align_base, end-align_base)) == NULL)
                        RETURN_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);

                    p_new_b->p_next = p_curr_b;
                    if (p_prev_b)
                        p_prev_b->p_next = p_new_b;
                    else
                        p_MM->free_blocks[i] = p_new_b;
                    break;
                }

                if ((align_base < p_curr_b->base) && (end >= p_curr_b->base))
                {
                    p_curr_b->base = align_base;
                }

                /* if size of the free block is less then alignment
                 * deletes that free block from the free list. */
                if ( (p_curr_b->end - p_curr_b->base) < alignment)
                {
                    if ( p_prev_b )
                        p_prev_b->p_next = p_curr_b->p_next;
                    else
                        p_MM->free_blocks[i] = p_curr_b->p_next;
                    fsl_os_free(p_curr_b);
                }
                break;
            }
            else
            {
                p_prev_b = p_curr_b;
                p_curr_b = p_curr_b->p_next;
            }
        }

        /* If no free block found to be updated, insert a new free block
         * to the end of the free list.
         */
        if ( !p_curr_b && ((((uint64_t)(end-base)) & ((uint64_t)(alignment-1))) == 0) )
        {
            if ((p_new_b = create_free_block(align_base, end-base)) == NULL)
                RETURN_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);

            if (p_prev_b)
                p_prev_b->p_next = p_new_b;
            else
                p_MM->free_blocks[i] = p_new_b;
        }

        /* Update boundaries of the new free block */
        if ((alignment == 1) && !p_new_b)
        {
            if ( p_curr_b && base > p_curr_b->base )
                base = p_curr_b->base;
            if ( p_curr_b && end < p_curr_b->end )
                end = p_curr_b->end;
        }
    }

    return (E_OK);
}

/****************************************************************
 *  Routine:      CutFree
 *
 *  Description:
 *      Cuts a free block from holdBase to holdEnd from the free lists.
 *      That is, it updates all free lists of the MM object do
 *      not include a block of memory from holdBase to holdEnd.
 *      For each free lists it seek for a free block that holds
 *      either holdBase or holdEnd. If such block is found it updates it.
 *
 *  Arguments:
 *      p_MM            - pointer to the MM object
 *      holdBase        - base address of the allocated block
 *      holdEnd         - end address of the allocated block
 *
 *  Return value:
 *      E_OK is returned on success,
 *      otherwise returns an error code.
 *
 ****************************************************************/
static int cut_free(t_MM *p_MM, uint64_t hold_base, uint64_t hold_end)
{
    t_free_block *p_prev_b, *p_curr_b, *p_new_b;
    uint64_t    align_base, base, end;
    uint64_t    alignment;
    int         i;

    for (i=0; i <= MM_MAX_ALIGNMENT; i++)
    {
        p_prev_b = p_new_b = 0;
        p_curr_b = p_MM->free_blocks[i];

        alignment = (uint64_t)(0x1 << i);
        align_base = MAKE_ALIGNED(hold_end, alignment);

        while ( p_curr_b )
        {
            base = p_curr_b->base;
            end = p_curr_b->end;

            if ( (hold_base <= base) && (hold_end <= end) && (hold_end > base) )
            {
                if ( align_base >= end ||
                     (align_base < end && ((end-align_base) < alignment)) )
                {
                    if (p_prev_b)
                        p_prev_b->p_next = p_curr_b->p_next;
                    else
                        p_MM->free_blocks[i] = p_curr_b->p_next;
                    fsl_os_free(p_curr_b);
                }
                else
                {
                    p_curr_b->base = align_base;
                }
                break;
            }
            else if ( (hold_base > base) && (hold_end <= end) )
            {
                if ( (hold_base-base) >= alignment )
                {
                    if ( (align_base < end) && ((end-align_base) >= alignment) )
                    {
                        if ((p_new_b = create_free_block(align_base, end-align_base)) == NULL)
                            RETURN_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
                        p_new_b->p_next = p_curr_b->p_next;
                        p_curr_b->p_next = p_new_b;
                    }
                    p_curr_b->end = hold_base;
                }
                else if ( (align_base < end) && ((end-align_base) >= alignment) )
                {
                    p_curr_b->base = align_base;
                }
                else
                {
                    if (p_prev_b)
                        p_prev_b->p_next = p_curr_b->p_next;
                    else
                        p_MM->free_blocks[i] = p_curr_b->p_next;
                    fsl_os_free(p_curr_b);
                }
                break;
            }
            else
            {
                p_prev_b = p_curr_b;
                p_curr_b = p_curr_b->p_next;
            }
        }
    }

    return (E_OK);
}

/****************************************************************
 *  Routine:     AddBusy
 *
 *  Description:
 *      Adds a new busy block to the list of busy blocks. Note,
 *      that all busy blocks are ordered by their base address in
 *      the busy list.
 *
 *  Arguments:
 *      MM              - handler to the MM object
 *      p_NewBusyB      - pointer to the a busy block
 *
 *  Return value:
 *      None.
 *
 ****************************************************************/
static void add_busy(t_MM *p_MM, t_busy_block *p_new_busy_b)
{
    t_busy_block *p_curr_busy_b, *p_prev_busy_b;

    /* finds a place of a new busy block in the list of busy blocks */
    p_prev_busy_b = 0;
    p_curr_busy_b = p_MM->busy_blocks;

    while ( p_curr_busy_b && p_new_busy_b->base > p_curr_busy_b->base )
    {
        p_prev_busy_b = p_curr_busy_b;
        p_curr_busy_b = p_curr_busy_b->p_next;
    }

    /* insert the new busy block into the list of busy blocks */
    if ( p_curr_busy_b )
        p_new_busy_b->p_next = p_curr_busy_b;
    if ( p_prev_busy_b )
        p_prev_busy_b->p_next = p_new_busy_b;
    else
        p_MM->busy_blocks = p_new_busy_b;
}

/****************************************************************
 *  Routine:    CutBusy
 *
 *  Description:
 *      Cuts a block from base to end from the list of busy blocks.
 *      This is done by updating the list of busy blocks do not
 *      include a given block, that block is going to be free. If a
 *      given block is a part of some other busy block, so that
 *      busy block is updated. If there are number of busy blocks
 *      included in the given block, so all that blocks are removed
 *      from the busy list and the end blocks are updated.
 *      If the given block devides some block into two parts, a new
 *      busy block is added to the busy list.
 *
 *  Arguments:
 *      p_MM  - pointer to the MM object
 *      base  - base address of a given busy block
 *      end   - end address of a given busy block
 *
 *  Return value:
 *      E_OK on success, E_NOMEMORY otherwise.
 *
 ****************************************************************/
static int cut_busy(t_MM *p_MM, uint64_t base, uint64_t end)
{
    t_busy_block  *p_curr_b, *p_prev_b, *p_new_b;

    p_curr_b = p_MM->busy_blocks;
    p_prev_b = p_new_b = 0;

    while ( p_curr_b )
    {
        if ( base < p_curr_b->end )
        {
            if ( end > p_curr_b->end )
            {
                t_busy_block *p_next_b;
                while ( p_curr_b->p_next && end >= p_curr_b->p_next->end )
                {
                    p_next_b = p_curr_b->p_next;
                    p_curr_b->p_next = p_curr_b->p_next->p_next;
                    fsl_os_free(p_next_b);
                }

                p_next_b = p_curr_b->p_next;
                if ( p_next_b && end > p_next_b->base )
                {
                    p_next_b->base = end;
                }
            }

            if ( base <= p_curr_b->base )
            {
                if ( end < p_curr_b->end && end > p_curr_b->base )
                {
                    p_curr_b->base = end;
                }
                else if ( end >= p_curr_b->end )
                {
                    if ( p_prev_b )
                        p_prev_b->p_next = p_curr_b->p_next;
                    else
                        p_MM->busy_blocks = p_curr_b->p_next;
                    fsl_os_free(p_curr_b);
                }
            }
            else
            {
                if ( end < p_curr_b->end && end > p_curr_b->base )
                {
                    if ((p_new_b = create_busy_block(end,
                                                  p_curr_b->end-end,
                                                  p_curr_b->name)) == NULL)
                        RETURN_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
                    p_new_b->p_next = p_curr_b->p_next;
                    p_curr_b->p_next = p_new_b;
                }
                p_curr_b->end = base;
            }
            break;
        }
        else
        {
            p_prev_b = p_curr_b;
            p_curr_b = p_curr_b->p_next;
        }
    }

    return (E_OK);
}

/****************************************************************
 *  Routine:     MmGetGreaterAlignment
 *
 *  Description:
 *      Allocates a block of memory according to the given size
 *      and the alignment. That routine is called from the MM_Get
 *      routine if the required alignment is greater then MM_MAX_ALIGNMENT.
 *      In that case, it goes over free blocks of 64 byte align list
 *      and checks if it has the required size of bytes of the required
 *      alignment. If no blocks found returns ILLEGAL_BASE.
 *      After the block is found and data is allocated, it calls
 *      the internal CutFree routine to update all free lists
 *      do not include a just allocated block. Of course, each
 *      free list contains a free blocks with the same alignment.
 *      It is also creates a busy block that holds
 *      information about an allocated block.
 *
 *  Arguments:
 *      MM              - handle to the MM object
 *      size            - size of the MM
 *      alignment       - index as a power of two defines
 *                        a required alignment that is greater then 64.
 *      name            - the name that specifies an allocated block.
 *
 *  Return value:
 *      base address of an allocated block.
 *      ILLEGAL_BASE if can't allocate a block
 *
 ****************************************************************/
static uint64_t slob_get_greater_alignment(t_MM *p_MM, uint64_t size, uint64_t alignment, char* name)
{
    t_free_block *p_free_b;
    t_busy_block *p_new_busy_b;
    uint64_t    hold_base, hold_end, align_base = 0;

    /* goes over free blocks of the 64 byte alignment list
       and look for a block of the suitable size and
       base address according to the alignment. */
    p_free_b = p_MM->free_blocks[MM_MAX_ALIGNMENT];

    while ( p_free_b )
    {
        align_base = MAKE_ALIGNED(p_free_b->base, alignment);

        /* the block is found if the aligned base inside the block
         * and has the anough size. */
        if ( align_base >= p_free_b->base &&
             align_base < p_free_b->end &&
             size <= (p_free_b->end - align_base) )
            break;
        else
            p_free_b = p_free_b->p_next;
    }

    /* If such block isn't found */
    if ( !p_free_b )
        return (uint64_t)(ILLEGAL_BASE);

    hold_base = align_base;
    hold_end = align_base + size;

    /* init a new busy block */
    if ((p_new_busy_b = create_busy_block(hold_base, size, name)) == NULL)
        return (uint64_t)(ILLEGAL_BASE);

    /* calls Update routine to update a lists of free blocks */
    if ( cut_free ( p_MM, hold_base, hold_end ) != E_OK )
        return (uint64_t)(ILLEGAL_BASE);

    /* insert the new busy block into the list of busy blocks */
    add_busy ( p_MM, p_new_busy_b );

    return (hold_base);
}


/**********************************************************************
 *                     MM API routines set                            *
 **********************************************************************/

/*****************************************************************************/
int slob_init(fsl_handle_t *slob, uint64_t base, uint64_t size)
{
    t_MM        *p_MM;
    uint64_t    new_base, new_size;
    int         i;

    if (!size)
    {
        RETURN_ERROR(MAJOR, E_INVALID_VALUE, ("size (should be positive)"));
    }

    /* Initializes a new MM object */
    p_MM = (t_MM *)fsl_os_malloc(sizeof(t_MM));
    if (!p_MM)
    {
        RETURN_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
    }

    p_MM->lock = spin_lock_create();
    if (!p_MM->lock)
    {
        fsl_os_free(p_MM);
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("MM spinlock!"));
    }

    /* Initializes counter of free memory to total size */
    p_MM->free_mem_size = size;

    /* Initializes a new memory block */
    if ((p_MM->mem_blocks = create_new_block(base, size)) == NULL)
        RETURN_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);

    /* A busy list is empty */
    p_MM->busy_blocks = 0;

    /* Initializes a new free block for each free list*/
    for (i=0; i <= MM_MAX_ALIGNMENT; i++)
    {
        new_base = MAKE_ALIGNED( base, (0x1 << i) );
        new_size = size - (new_base - base);

        if ((p_MM->free_blocks[i] = create_free_block(new_base, new_size)) == NULL)
            RETURN_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
    }

    *slob = p_MM;

    return (E_OK);
}

/*****************************************************************************/
void slob_free(fsl_handle_t slob)
{
    t_MM        *p_MM = (t_MM *)slob;
    t_mem_block  *p_mem_block;
    t_busy_block *p_busy_block;
    t_free_block *p_free_block;
    void        *p_block;
    int         i;

    ASSERT_COND(p_MM);

    /* release memory allocated for busy blocks */
    p_busy_block = p_MM->busy_blocks;
    while ( p_busy_block )
    {
        p_block = p_busy_block;
        p_busy_block = p_busy_block->p_next;
        fsl_os_free(p_block);
    }

    /* release memory allocated for free blocks */
    for (i=0; i <= MM_MAX_ALIGNMENT; i++)
    {
        p_free_block = p_MM->free_blocks[i];
        while ( p_free_block )
        {
            p_block = p_free_block;
            p_free_block = p_free_block->p_next;
            fsl_os_free(p_block);
        }
    }

    /* release memory allocated for memory blocks */
    p_mem_block = p_MM->mem_blocks;
    while ( p_mem_block )
    {
        p_block = p_mem_block;
        p_mem_block = p_mem_block->p_next;
        fsl_os_free(p_block);
    }

    if (p_MM->lock)
        spin_lock_free(p_MM->lock);

    /* release memory allocated for MM object itself */
    fsl_os_free(p_MM);
}

/*****************************************************************************/
uint64_t slob_get(fsl_handle_t slob, uint64_t size, uint64_t alignment, char* name)
{
    t_MM        *p_MM = (t_MM *)slob;
    t_free_block *p_free_b;
    t_busy_block *p_new_busy_b;
    uint64_t    hold_base, hold_end, j, i = 0;
    uint32_t    int_flags;

    /* checks that alignment value is greater then zero */
    if (alignment == 0)
    {
        alignment = 1;
    }

    j = alignment;

    /* checks if alignment is a power of two, if it correct and if the
       required size is multiple of the given alignment. */
    while ((j & 0x1) == 0)
    {
        i++;
        j = j >> 1;
    }

    /* if the given alignment isn't power of two, returns an error */
    if (j != 1)
    {
        REPORT_ERROR(MAJOR, E_INVALID_VALUE, ("alignment (should be power of 2)"));
        return (uint64_t)ILLEGAL_BASE;
    }

    if (i > MM_MAX_ALIGNMENT)
    {
        return (slob_get_greater_alignment(p_MM, size, alignment, name));
    }

    int_flags = spin_lock_irqsave(p_MM->lock);
    /* look for a block of the size greater or equal to the required size. */
    p_free_b = p_MM->free_blocks[i];
    while ( p_free_b && (p_free_b->end - p_free_b->base) < size )
        p_free_b = p_free_b->p_next;

    /* If such block is found */
    if ( !p_free_b )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    hold_base = p_free_b->base;
    hold_end = hold_base + size;

    /* init a new busy block */
    if ((p_new_busy_b = create_busy_block(hold_base, size, name)) == NULL)
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    /* calls Update routine to update a lists of free blocks */
    if ( cut_free ( p_MM, hold_base, hold_end ) != E_OK )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    /* Decreasing the allocated memory size from free memory size */
    p_MM->free_mem_size -= size;

    /* insert the new busy block into the list of busy blocks */
    add_busy ( p_MM, p_new_busy_b );
    spin_unlock_irqrestore(p_MM->lock, int_flags);

    return (hold_base);
}

/*****************************************************************************/
uint64_t slob_get_force(fsl_handle_t slob, uint64_t base, uint64_t size, char* name)
{
    t_MM        *p_MM = (t_MM *)slob;
    t_free_block *p_free_b;
    t_busy_block *p_new_busy_b;
    uint32_t    int_flags;
    int         block_is_free = 0;

    ASSERT_COND(p_MM);

    int_flags = spin_lock_irqsave(p_MM->lock);
    p_free_b = p_MM->free_blocks[0]; /* The biggest free blocks are in the
                                      free list with alignment 1 */

    while ( p_free_b )
    {
        if ( base >= p_free_b->base && (base+size) <= p_free_b->end )
        {
            block_is_free = 1;
            break;
        }
        else
            p_free_b = p_free_b->p_next;
    }

    if ( !block_is_free )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    /* init a new busy block */
    if ((p_new_busy_b = create_busy_block(base, size, name)) == NULL)
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    /* calls Update routine to update a lists of free blocks */
    if ( cut_free ( p_MM, base, base+size ) != E_OK )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    /* Decreasing the allocated memory size from free memory size */
    p_MM->free_mem_size -= size;

    /* insert the new busy block into the list of busy blocks */
    add_busy ( p_MM, p_new_busy_b );
    spin_unlock_irqrestore(p_MM->lock, int_flags);

    return (base);
}

/*****************************************************************************/
uint64_t slob_get_force_min(fsl_handle_t slob, uint64_t size, uint64_t alignment, uint64_t min, char* name)
{
    t_MM        *p_MM = (t_MM *)slob;
    t_free_block *p_free_b;
    t_busy_block *p_new_busy_b;
    uint64_t    hold_base, hold_end, j = alignment, i=0;
    uint32_t    int_flags;

    ASSERT_COND(p_MM);

    /* checks if alignment is a power of two, if it correct and if the
       required size is multiple of the given alignment. */
    while ((j & 0x1) == 0)
    {
        i++;
        j = j >> 1;
    }

    if ( (j != 1) || (i > MM_MAX_ALIGNMENT) )
    {
        return (uint64_t)(ILLEGAL_BASE);
    }

    int_flags = spin_lock_irqsave(p_MM->lock);
    p_free_b = p_MM->free_blocks[i];

    /* look for the first block that contains the minimum
       base address. If the whole required size may be fit
       into it, use that block, otherwise look for the next
       block of size greater or equal to the required size. */
    while ( p_free_b && (min >= p_free_b->end))
            p_free_b = p_free_b->p_next;

    /* If such block is found */
    if ( !p_free_b )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    /* if this block is large enough, use this block */
    hold_base = ( min <= p_free_b->base ) ? p_free_b->base : min;
    if ((hold_base + size) <= p_free_b->end )
    {
        hold_end = hold_base + size;
    }
    else
    {
        p_free_b = p_free_b->p_next;
        while ( p_free_b && ((p_free_b->end - p_free_b->base) < size) )
            p_free_b = p_free_b->p_next;

        /* If such block is found */
        if ( !p_free_b )
        {
            spin_unlock_irqrestore(p_MM->lock, int_flags);
            return (uint64_t)(ILLEGAL_BASE);
        }

        hold_base = p_free_b->base;
        hold_end = hold_base + size;
    }

    /* init a new busy block */
    if ((p_new_busy_b = create_busy_block(hold_base, size, name)) == NULL)
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    /* calls Update routine to update a lists of free blocks */
    if ( cut_free( p_MM, hold_base, hold_end ) != E_OK )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(ILLEGAL_BASE);
    }

    /* Decreasing the allocated memory size from free memory size */
    p_MM->free_mem_size -= size;

    /* insert the new busy block into the list of busy blocks */
    add_busy( p_MM, p_new_busy_b );
    spin_unlock_irqrestore(p_MM->lock, int_flags);

    return (hold_base);
}

/*****************************************************************************/
uint64_t slob_put(fsl_handle_t slob, uint64_t base)
{
    t_MM        *p_MM = (t_MM *)slob;
    t_busy_block *p_busy_b, *p_prev_busy_b;
    uint64_t    size;
    uint32_t    int_flags;

    ASSERT_COND(p_MM);

    /* Look for a busy block that have the given base value.
     * That block will be returned back to the memory.
     */
    p_prev_busy_b = 0;

    int_flags = spin_lock_irqsave(p_MM->lock);
    p_busy_b = p_MM->busy_blocks;
    while ( p_busy_b && base != p_busy_b->base )
    {
        p_prev_busy_b = p_busy_b;
        p_busy_b = p_busy_b->p_next;
    }

    if ( !p_busy_b )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(0);
    }

    if ( add_free( p_MM, p_busy_b->base, p_busy_b->end ) != E_OK )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(0);
    }

    /* removes a busy block form the list of busy blocks */
    if ( p_prev_busy_b )
        p_prev_busy_b->p_next = p_busy_b->p_next;
    else
        p_MM->busy_blocks = p_busy_b->p_next;

    size = p_busy_b->end - p_busy_b->base;

    /* Adding the deallocated memory size to free memory size */
    p_MM->free_mem_size += size;

    fsl_os_free(p_busy_b);
    spin_unlock_irqrestore(p_MM->lock, int_flags);

    return (size);
}

/*****************************************************************************/
uint64_t slob_put_force(fsl_handle_t slob, uint64_t base, uint64_t size)
{
    t_MM        *p_MM = (t_MM *)slob;
    uint64_t    end = base + size;
    uint32_t    int_flags;

    ASSERT_COND(p_MM);

    int_flags = spin_lock_irqsave(p_MM->lock);

    if ( cut_busy( p_MM, base, end ) != E_OK )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(0);
    }

    if ( add_free ( p_MM, base, end ) != E_OK )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        return (uint64_t)(0);
    }

    /* Adding the deallocated memory size to free memory size */
    p_MM->free_mem_size += size;

    spin_unlock_irqrestore(p_MM->lock, int_flags);

    return (size);
}

/*****************************************************************************/
int slob_add(fsl_handle_t slob, uint64_t base, uint64_t size)
{
    t_MM        *p_MM = (t_MM *)slob;
    t_mem_block  *p_mem_b, *p_new_mem_b;
    int     err_code;
    uint32_t    int_flags;

    ASSERT_COND(p_MM);

    /* find a last block in the list of memory blocks to insert a new
     * memory block
     */
    int_flags = spin_lock_irqsave(p_MM->lock);

    p_mem_b = p_MM->mem_blocks;
    while ( p_mem_b->p_next )
    {
        if ( base >= p_mem_b->base && base < p_mem_b->end )
        {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
            RETURN_ERROR(MAJOR, E_ALREADY_EXISTS, NO_MSG);
        }
        p_mem_b = p_mem_b->p_next;
    }
    /* check for a last memory block */
    if ( base >= p_mem_b->base && base < p_mem_b->end )
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        RETURN_ERROR(MAJOR, E_ALREADY_EXISTS, NO_MSG);
    }

    /* create a new memory block */
    if ((p_new_mem_b = create_new_block(base, size)) == NULL)
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        RETURN_ERROR(MAJOR, E_NO_MEMORY, NO_MSG);
    }

    /* append a new memory block to the end of the list of memory blocks */
    p_mem_b->p_next = p_new_mem_b;

    /* add a new free block to the free lists */
    err_code = add_free(p_MM, base, base+size);
    if (err_code)
    {
        spin_unlock_irqrestore(p_MM->lock, int_flags);
        p_mem_b->p_next = 0;
        fsl_os_free(p_new_mem_b);
        return (err_code);
    }

    /* Adding the new block size to free memory size */
    p_MM->free_mem_size += size;

    spin_unlock_irqrestore(p_MM->lock, int_flags);

    return (E_OK);
}

/*****************************************************************************/
uint64_t slob_get_base(fsl_handle_t slob)
{
    t_MM        *p_MM = (t_MM*)slob;
    t_mem_block *p_mem_block;

    ASSERT_COND(p_MM);

    p_mem_block = p_MM->mem_blocks;
    return  p_mem_block->base;
}

/*****************************************************************************/
int slob_in_range(fsl_handle_t slob, uint64_t addr)
{
    t_MM       *p_MM = (t_MM*)slob;
    t_mem_block *p_mem_block;

    ASSERT_COND(p_MM);

    p_mem_block = p_MM->mem_blocks;

    if ((addr >= p_mem_block->base) && (addr < p_mem_block->end))
        return 1;
    else
        return 0;
}

/*****************************************************************************/
uint64_t slob_get_free_mem_size(fsl_handle_t slob)
{
    t_MM       *p_MM = (t_MM*)slob;

    ASSERT_COND(p_MM);

    return p_MM->free_mem_size;
}

/*****************************************************************************/
void slob_dump(fsl_handle_t slob)
{
    t_MM        *p_MM = (t_MM *)slob;
    t_free_block *p_free_b;
    t_busy_block *p_busy_b;
    int          i;

    p_busy_b = p_MM->busy_blocks;
    pr_debug("list of busy blocks:\n");
    while (p_busy_b)
    {
        pr_debug("\t0x%p: (%s: b=0x%llx, e=0x%llx)\n", p_busy_b, p_busy_b->name, p_busy_b->base, p_busy_b->end );
        p_busy_b = p_busy_b->p_next;
    }

    pr_debug("\n_lists of free blocks according to alignment:\n");
    for (i=0; i <= MM_MAX_ALIGNMENT; i++)
    {
        pr_debug("%d alignment:\n", (0x1 << i));
        p_free_b = p_MM->free_blocks[i];
        while (p_free_b)
        {
            pr_debug("\t0x%p: (b=0x%llx, e=0x%llx)\n", p_free_b, p_free_b->base, p_free_b->end);
            p_free_b = p_free_b->p_next;
        }
        pr_debug("\n");
    }
}

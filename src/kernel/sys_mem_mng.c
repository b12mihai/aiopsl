
#include "common/fsl_malloc.h"
#include "kernel/fsl_spinlock.h"
#include "fsl_dbg.h"

#include "inc/mem_mng_util.h"
#include "fsl_mem_mng.h"
#include "sys.h"


#ifdef VERILOG
#if (defined(__GNUC__)) && (defined(sun))
extern int sigblock(int);
extern int sigsetmask(int);
#else
#define sigblock(i)     0
#define sigsetmask(i)
#endif /* GNUC */
#endif /* VERILOG */


typedef struct t_sys_virt_mem_map
{
    uint64_t    virt_addr;
    uint64_t    phys_addr;
    uint64_t    size;
    list_t      node;
} t_sys_virt_mem_map;

#define VIRT_MEM_MAP(ptr)   LIST_OBJECT(ptr, t_sys_virt_mem_map, node)

static t_sys_virt_mem_map * sys_find_virt_addr_mapping(uint64_t virt_addr);
static t_sys_virt_mem_map * sys_find_phys_addr_mapping(uint64_t phys_addr);

static void *   sys_default_malloc(uint32_t size);
static void     sys_default_free(void *p_memory);
static void *   sys_aligned_malloc(uint32_t size, uint32_t alignment);
static void     sys_aligned_free(void *p_memory);

static void     sys_print_mem_leak(void        *p_memory,
                                uint32_t    size,
                                char        *info,
                                char        *filename,
                                int         line);

/* Global System Object */
extern t_system sys;


/*****************************************************************************/
int sys_register_virt_mem_mapping(uint64_t virt_addr, uint64_t phys_addr, uint64_t size)
{
    t_sys_virt_mem_map *p_virt_mem_map;
#ifndef AIOP
    uint32_t        int_flags;
#endif /* AIOP */
    p_virt_mem_map = (t_sys_virt_mem_map *)fsl_os_malloc(sizeof(t_sys_virt_mem_map));
    if (!p_virt_mem_map)
        RETURN_ERROR(MAJOR, E_NO_MEMORY, ("virtual memory mapping entry"));

    p_virt_mem_map->virt_addr = virt_addr;
    p_virt_mem_map->phys_addr = phys_addr;
    p_virt_mem_map->size     = size;

    INIT_LIST(&p_virt_mem_map->node);

#ifdef AIOP
    lock_spinlock(&(sys.virt_mem_lock));
#else /* not AIOP */
    int_flags = spin_lock_irqsave(&(sys.virt_mem_lock));
#endif /* AIOP */
    list_add_to_tail(&p_virt_mem_map->node, &(sys.virt_mem_list));
#ifdef AIOP
    unlock_spinlock(&(sys.virt_mem_lock));
#else /* not AIOP */
    spin_unlock_irqrestore(&(sys.virt_mem_lock), int_flags);
#endif /* AIOP */
    
    return E_OK;
}


/*****************************************************************************/
int sys_unregister_virt_mem_mapping(uint64_t virt_addr)
{
#ifndef AIOP
    uint32_t        int_flags;
#endif /* AIOP */
    t_sys_virt_mem_map *p_virt_mem_map = sys_find_virt_addr_mapping(virt_addr);

    if (!p_virt_mem_map)
        RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("virtual address"));
#ifdef AIOP
    lock_spinlock(&(sys.virt_mem_lock));
#else /* not AIOP */
    int_flags = spin_lock_irqsave(&(sys.virt_mem_lock));
#endif /* AIOP */
    list_del(&(p_virt_mem_map->node));
    fsl_os_free(p_virt_mem_map);
#ifdef AIOP
    unlock_spinlock(&(sys.virt_mem_lock));
#else /* not AIOP */
    spin_unlock_irqrestore(&(sys.virt_mem_lock), int_flags);
#endif /* AIOP */
    
    return E_OK;
}


/*****************************************************************************/
dma_addr_t sys_virt_to_phys(void *virt_addr)
{
    t_sys_virt_mem_map *p_virt_mem_map;
    uint64_t        virt_addr64 = PTR_TO_UINT(virt_addr);

    p_virt_mem_map = sys_find_virt_addr_mapping(virt_addr64);
    if (p_virt_mem_map)
    {
        /* This is optimization - put the latest in the list-head - like a cache */
        if (sys.virt_mem_list.next != &p_virt_mem_map->node)
        {
#ifdef AIOP
            lock_spinlock(&(sys.virt_mem_lock));
#else /* not AIOP */
            uint32_t int_flags = spin_lock_irqsave(&(sys.virt_mem_lock));
#endif /* AIOP */
            list_del_and_init(&p_virt_mem_map->node);
            list_add(&p_virt_mem_map->node, &(sys.virt_mem_list));
#ifdef AIOP
            unlock_spinlock(&(sys.virt_mem_lock));
#else /* not AIOP */
            spin_unlock_irqrestore(&(sys.virt_mem_lock), int_flags);
#endif /* AIOP */
        }
        return (dma_addr_t)
            (virt_addr64 - p_virt_mem_map->virt_addr + p_virt_mem_map->phys_addr);
    }

    /* Mapping not found */
    return (dma_addr_t)virt_addr64;
}


/*****************************************************************************/
void * sys_phys_to_virt(dma_addr_t phys_addr)
{
    t_sys_virt_mem_map *p_virt_mem_map;

    p_virt_mem_map = sys_find_phys_addr_mapping((uint64_t)phys_addr);
    if (p_virt_mem_map)
    {
        /* This is optimization - put the latest in the list-head - like a cache */
        if (sys.virt_mem_list.next != &p_virt_mem_map->node)
        {
#ifdef AIOP
            lock_spinlock(&(sys.virt_mem_lock));
#else /* not AIOP */
            uint32_t int_flags = spin_lock_irqsave(&(sys.virt_mem_lock));
#endif /* AIOP */
            list_del_and_init(&p_virt_mem_map->node);
            list_add(&p_virt_mem_map->node, &(sys.virt_mem_list));
#ifdef AIOP
            unlock_spinlock(&(sys.virt_mem_lock));
#else /* not AIOP */
            spin_unlock_irqrestore(&(sys.virt_mem_lock), int_flags);
#endif /* AIOP */
        }
        return UINT_TO_PTR(phys_addr - p_virt_mem_map->phys_addr + p_virt_mem_map->virt_addr);
    }

    /* Mapping not found */
    return UINT_TO_PTR(phys_addr);
}


/*****************************************************************************/
static t_sys_virt_mem_map * sys_find_virt_addr_mapping(uint64_t virt_addr)
{
    t_sys_virt_mem_map *p_virt_mem_map;
    list_t          *p_pos;

    LIST_FOR_EACH(p_pos, &(sys.virt_mem_list))
    {
        p_virt_mem_map = VIRT_MEM_MAP(p_pos);

        if ((virt_addr >= p_virt_mem_map->virt_addr) &&
            (virt_addr <  p_virt_mem_map->virt_addr + p_virt_mem_map->size))
        {
            return p_virt_mem_map;
        }
    }

    return NULL;
}


/*****************************************************************************/
static t_sys_virt_mem_map * sys_find_phys_addr_mapping(uint64_t phys_addr)
{
    t_sys_virt_mem_map *p_virt_mem_map = NULL;
    list_t          *p_pos;

    LIST_FOR_EACH(p_pos, &(sys.virt_mem_list))
    {
        p_virt_mem_map = VIRT_MEM_MAP(p_pos);

        if ((phys_addr >= p_virt_mem_map->phys_addr) &&
            (phys_addr <  p_virt_mem_map->phys_addr + p_virt_mem_map->size))
            return p_virt_mem_map;
    }

    return NULL;
}


/*****************************************************************************/
void * sys_mem_alloc(int         partition_id,
                    uint32_t    size,
                    uint32_t    alignment,
                    char        *info,
                    char        *filename,
                    int         line)
{
    void *p_memory;

    ASSERT_COND(sys.mem_mng);
    ASSERT_COND(size > 0);

    if (partition_id == SYS_DEFAULT_HEAP_PARTITION)
    {
        /* Use registered heap partition
           (set to MEM_MNG_EARLY_PARTITION_ID when no partitions are registered) */
        partition_id = sys.heap_partition_id;
    }

    p_memory = mem_mng_alloc_mem(sys.mem_mng,
                                partition_id,
                                size,
                                alignment,
                                info,
                                filename,
                                line);

    return p_memory;
}


/*****************************************************************************/
void sys_mem_free(void *p_memory)
{
    ASSERT_COND(sys.mem_mng);

    mem_mng_free_mem(sys.mem_mng, p_memory);
}


/*****************************************************************************/
int sys_get_available_mem_partition(void)
{
    int partition_id;

    ASSERT_COND(sys.mem_mng);

    partition_id = mem_mng_get_available_partition_id(sys.mem_mng);

    return partition_id;
}


/*****************************************************************************/
int sys_register_mem_partition(int        partition_id,
                                 uintptr_t  base_address,
                                 uint64_t   size,
                                 uint32_t   attributes,
                                 char       name[],
                                 void *     (*f_user_malloc)(uint32_t size, uint32_t alignment),
                                 void       (*f_user_free)(void *p_addr),
                                 int        enable_debug)
{
    int err_code;
    int    is_heap_partition = 0;

    ASSERT_COND(sys.mem_mng);

    if (partition_id == SYS_DEFAULT_HEAP_PARTITION)
    {
        RETURN_ERROR(MAJOR, E_INVALID_VALUE,
                     ("partition ID %d is reserved for default heap",
                      SYS_DEFAULT_HEAP_PARTITION));
    }

    /* Check if matches the default heap partition */
    if ((sys.heap_addr >= base_address) && (sys.heap_addr < (base_address + size)))
    {
        is_heap_partition = 1;

        if (f_user_malloc || f_user_free)
            RETURN_ERROR(MAJOR, E_INVALID_OPERATION,
                         ("cannot override malloc/free routines of default heap"));

        f_user_malloc = sys_aligned_malloc;
        f_user_free = sys_aligned_free;
    }

    err_code = mem_mng_register_partition(sys.mem_mng,
                                        partition_id,
                                        base_address,
                                        size,
                                        attributes,
                                        name,
                                        f_user_malloc,
                                        f_user_free,
                                        enable_debug);
    if (err_code != E_OK)
    {
        RETURN_ERROR(MAJOR, err_code, NO_MSG);
    }

    if (is_heap_partition)
    {
        /* From this point we can trace the heap partition */
        sys.heap_partition_id = partition_id;
    }

    return E_OK;
}


/*****************************************************************************/
int sys_unregister_mem_partition(int partition_id)
{
    t_mem_mng_partition_info   partition_info;
    uint32_t                leaks_count;
    int                 err_code;

    ASSERT_COND(sys.mem_mng);

    leaks_count = mem_mng_check_leaks(sys.mem_mng, partition_id, NULL);

    if (leaks_count)
    {
        /* Print memory leaks for this partition */
        mem_mng_get_partition_info(sys.mem_mng, partition_id, &partition_info);

        pr_info("\r\n_memory leaks report - %s:\r\n", partition_info.name);
        pr_info("------------------------------------------------------------\r\n");
        mem_mng_check_leaks(sys.mem_mng, partition_id, sys_print_mem_leak);
        pr_info("------------------------------------------------------------\r\n");
    }

    err_code = mem_mng_unregister_partition(sys.mem_mng, partition_id);

    if (err_code != E_OK)
    {
        RETURN_ERROR(MAJOR, err_code, NO_MSG);
    }

    if (partition_id == sys.heap_partition_id)
    {
        /* Reset value of default heap partition */
        sys.heap_partition_id = MEM_MNG_EARLY_PARTITION_ID;
    }

    return E_OK;
}


/*****************************************************************************/
uint64_t sys_get_mem_partition_base(int partition_id)
{
    t_mem_mng_partition_info   partition_info;
    int                 err_code;

    ASSERT_COND(sys.mem_mng);

    if (partition_id == SYS_DEFAULT_HEAP_PARTITION)
    {
        partition_id = sys.heap_partition_id;
    }

    err_code = mem_mng_get_partition_info(sys.mem_mng, partition_id, &partition_info);

    if (err_code != E_OK)
    {
        REPORT_ERROR(MAJOR, err_code, NO_MSG);
        return (uint32_t)ILLEGAL_BASE;
    }

    return partition_info.base_address;
}


/*****************************************************************************/
uint32_t sys_get_mem_partition_attributes(int partition_id)
{
    t_mem_mng_partition_info   partition_info;
    int                 err_code;

    ASSERT_COND(sys.mem_mng);

    if (partition_id == SYS_DEFAULT_HEAP_PARTITION)
    {
        partition_id = sys.heap_partition_id;
    }

    err_code = mem_mng_get_partition_info(sys.mem_mng, partition_id, &partition_info);

    if (err_code != E_OK)
    {
        REPORT_ERROR(MAJOR, err_code, NO_MSG);
        return (uint32_t)0;
    }

    return partition_info.attributes;
}


/*****************************************************************************/
void sys_print_mem_partition_debug_info(int partition_id, int report_leaks)
{
    t_mem_mng_partition_info   partition_info;
    uint32_t                leaks_count;

    ASSERT_COND(sys.mem_mng);

    if (partition_id == SYS_DEFAULT_HEAP_PARTITION)
    {
        partition_id = sys.heap_partition_id;
    }

    mem_mng_get_partition_info(sys.mem_mng, partition_id, &partition_info);

    pr_info("\r\n_memory usage - %s%s:\r\n",
             partition_info.name,
             ((partition_id == sys.heap_partition_id) ? " (default heap)" : ""));
    pr_info("------------------------------------------------------------\r\n");
    pr_info("base address:         0x%08X\r\n", partition_info.base_address);
    pr_info("total size (KB):      %10lu\r\n", (partition_info.size / 1024));
    pr_info("current usage (KB):   %10lu\r\n", (partition_info.current_usage / 1024));
    pr_info("maximum usage (KB):   %10lu\r\n", (partition_info.maximum_usage / 1024));
    pr_info("total allocations:    %10lu\r\n", partition_info.total_allocations);
    pr_info("total deallocations:  %10lu\r\n", partition_info.total_deallocations);
    pr_info("\r\n");

    if (report_leaks)
    {
        pr_info("\r\n_memory leaks report - %s:\r\n", partition_info.name);
        pr_info("------------------------------------------------------------\r\n");

        leaks_count = mem_mng_check_leaks(sys.mem_mng, partition_id, sys_print_mem_leak);

        if (!leaks_count)
        {
            pr_info("no memory leaks were found\r\n");
        }
        pr_info("\r\n");
    }
}


/*****************************************************************************/
int sys_init_memory_management(void)
{
    t_mem_mng_param mem_mng_param;
#ifdef AIOP
    sys.virt_mem_lock = 0;
    sys.mem_mng_lock = 0;
    sys.mem_part_mng_lock = 0;
#else /* not AIOP */
    spin_lock_init(&(sys.virt_mem_lock));
    spin_lock_init(&(sys.mem_mng_lock));
    spin_lock_init(&(sys.mem_part_mng_lock)); 
#endif /* AIOP */
    
    INIT_LIST(&(sys.virt_mem_list));

    /* Initialize memory allocation manager module */
    mem_mng_param.f_malloc = sys_default_malloc;
    mem_mng_param.f_free = sys_default_free;
    mem_mng_param.f_early_malloc = sys_aligned_malloc;
    mem_mng_param.f_early_free = sys_aligned_free;
    mem_mng_param.lock = &(sys.mem_part_mng_lock);

    sys.mem_mng = mem_mng_init(&mem_mng_param);
    if (!sys.mem_mng)
    {
        RETURN_ERROR(MAJOR, E_NOT_AVAILABLE, ("memory management object"));
    }

    /* Temporary allocation for identifying the default heap region */
    sys.heap_addr = (uintptr_t)sys_default_malloc(8);
    sys_default_free((void *)sys.heap_addr);

    sys.heap_partition_id = MEM_MNG_EARLY_PARTITION_ID;

    return E_OK;
}


/*****************************************************************************/
int sys_free_memory_management(void)
{
    uint32_t leaks_count;

    if (sys.mem_mng)
    {
        leaks_count = mem_mng_check_leaks(sys.mem_mng, MEM_MNG_EARLY_PARTITION_ID, NULL);

        if (leaks_count)
        {
            /* Print memory leaks of early allocations */
            pr_info("\r\n_memory leaks report - early allocations:\r\n");
            pr_info("------------------------------------------------------------\r\n");
            mem_mng_check_leaks(sys.mem_mng, MEM_MNG_EARLY_PARTITION_ID, sys_print_mem_leak);
            pr_info("------------------------------------------------------------\r\n");
        }

        mem_mng_free(sys.mem_mng);
        sys.mem_mng = NULL;
    }

    return E_OK;
}

/*****************************************************************************/
static void * sys_default_malloc(uint32_t size)
{
    void        *p;
#ifdef VERILOG
    int         i = sigblock(0x00002000);   /* Block SIGTSTP signal -
                                               stop signal generated from keyboard */
    p = (void *)malloc(size);
    sigsetmask(i);                          /* Set SIGTSTP signal back */
#else  /* not VERILOG */
#ifdef AIOP
    lock_spinlock(&(sys.mem_mng_lock));
#else /* not AIOP */
    uint32_t    int_flags;
    int_flags = spin_lock_irqsave(&(sys.mem_mng_lock));
#endif /* AIOP */
    p = (void *)malloc(size);
#ifdef AIOP
    unlock_spinlock(&(sys.mem_mng_lock));
#else /* not AIOP */
    spin_unlock_irqrestore(&(sys.mem_mng_lock), int_flags);
#endif /* AIOP */
#endif /* not VERILOG */

    return p;
}


/*****************************************************************************/
static void sys_default_free(void *p_memory)
{
#ifdef VERILOG
    int i = sigblock(0x00002000);   /* Block SIGTSTP signal -
                                       stop signal generated from keyboard */
    free(p_memory);
    sigsetmask(i);                  /* Set SIGTSTP signal back */
#else /* not VERILOG */
#ifndef AIOP
    uint32_t    int_flags;
#endif /* AIOP */
    
#ifdef AIOP
    lock_spinlock(&(sys.mem_mng_lock));
#else /* not AIOP */
    int_flags = spin_lock_irqsave(&(sys.mem_mng_lock));
#endif /* AIOP */
    free(p_memory);
#ifdef AIOP
    unlock_spinlock(&(sys.mem_mng_lock));
#else /* not AIOP */
    spin_unlock_irqrestore(&(sys.mem_mng_lock), int_flags);
#endif /* AIOP */
#endif /* VERILOG */
}

/*****************************************************************************/
static void * sys_aligned_malloc(uint32_t size, uint32_t alignment)
{
    uintptr_t    alloc_addr, aligned_addr;
    if (alignment < sizeof(uintptr_t))
        alignment = sizeof(uintptr_t);

    alloc_addr = (uintptr_t)sys_default_malloc(size + alignment);
    if (alloc_addr == 0)
    {
        REPORT_ERROR(MINOR, E_NO_MEMORY, ("default heap"));
        return NULL;
    }

    /* Reasonable assumption:
       Memory returned from default heap is at least sizeof(uintptr_t) bytes aligned */
    ASSERT_COND((alloc_addr & (sizeof(uintptr_t)-1)) == 0);
    /* Store the real allocated address in the alignment area */
    aligned_addr = (uintptr_t)((alloc_addr + alignment) & ~((uintptr_t)alignment - 1));
    *(uintptr_t *)(aligned_addr - sizeof(uintptr_t)) = alloc_addr;

    return UINT_TO_PTR(aligned_addr);
}

/*****************************************************************************/
static void sys_aligned_free(void *p_memory)
{
    /* Find allocated address from aligned address */
    uintptr_t alloc_addr = *(uintptr_t *)(((uintptr_t)p_memory) - sizeof(uintptr_t));

    sys_default_free(UINT_TO_PTR(alloc_addr));
}


/*****************************************************************************/
static void sys_print_mem_leak(void        *p_memory,
                            uint32_t    size,
                            char        *info,
                            char        *filename,
                            int         line)
{
    UNUSED(size);
    UNUSED(info);

    pr_info("memory leak: 0x%09p, file: %s (%d)\r\n", p_memory, filename, line);
}



/******************************************************************************

 @File          platform_ext.h

 @Description   Prototypes, externals and typedefs for platform routines
****************************************************************************/
#ifndef __FSL_PLATFORM_H
#define __FSL_PLATFORM_H

#include "common/types.h"
#include "common/errors.h"
#include "arch/fsl_soc.h"


/**************************************************************************//**
 @Group         platform_grp PLATFORM Application Programming Interface

 @Description   Generic Platform API, that must be implemented by each
                specific platform.

 @{
*//***************************************************************************/

#define PLATFORM_IO_MODE_ANY            (~0)    /**< Unspecified I/O mode */

#define MAX_CHIP_NAME_LEN               10   /**< Maximum chip name length (including null character) */


/**************************************************************************//**
 @Description   Cache Operation Mode
*//***************************************************************************/
typedef enum cache_mode {
    E_CACHE_MODE_DISABLED       = 0x00000000,   /**< Cache is disabled */
    E_CACHE_MODE_DATA_ONLY      = 0x00000001,   /**< Cache is enabled for data only */
    E_CACHE_MODE_INST_ONLY      = 0x00000002,   /**< Cache is enabled for instructions only */
    E_CACHE_MODE_DATA_AND_INST  = 0x00000003    /**< Cache is enabled for data and instructions */
} e_cache_mode;

/**************************************************************************//**
 @Description   Mapped memory type for obtaining module's base address
*//***************************************************************************/
typedef enum mapped_mem_type {
    E_MAPPED_MEM_TYPE_GEN_REGS,
    E_MAPPED_MEM_TYPE_ATMU_REGS,
    E_MAPPED_MEM_TYPE_PCI_MSI_REGS,
    E_MAPPED_MEM_TYPE_MII_MNG_REGS,
    E_MAPPED_MEM_TYPE_TDM_DMA_REGS,
    E_MAPPED_MEM_TYPE_MURAM,
    E_MAPPED_MEM_TYPE_SI_RAM,
    E_MAPPED_MEM_TYPE_PAR_IO_INTR_REGS,
    E_MAPPED_MEM_TYPE_MC_PORTAL,
    E_MAPPED_MEM_TYPE_CE_PORTAL,
    E_MAPPED_MEM_TYPE_CI_PORTAL
} e_mapped_mem_type;

/**************************************************************************//**
 @Description   Platform Events
*//***************************************************************************/
typedef enum platform_event {
    E_PLATFORM_EVENT_LINK_DOWN, /**< Link-up event */
    E_PLATFORM_EVENT_LINK_UP    /**< Link-down event */
} e_platform_event;

/**************************************************************************//**
 @Description   Interrupt Source Types
*//***************************************************************************/
typedef enum interrupt_type {
    E_INTR_TYPE_GENERAL,
    E_INTR_TYPE_IRQ,
    E_INTR_TYPE_IPI,
    E_INTR_TYPE_MSG,
    E_INTR_TYPE_ERR,
    E_INTR_TYPE_GTIMERS_TIMER,
    E_INTR_TYPE_RTC_COUNT,
    E_INTR_TYPE_RTC_ALARM,
    E_INTR_TYPE_PCI_IRQ,
    E_INTR_TYPE_PCI_PME,
    E_INTR_TYPE_PCI_MSI,
    E_INTR_TYPE_ETSEC_TX,
    E_INTR_TYPE_ETSEC_RX,
    E_INTR_TYPE_ETSEC_ERROR,
    E_INTR_TYPE_RIO_PW,
    E_INTR_TYPE_RIO_DB_OUT,
    E_INTR_TYPE_RIO_DB_IN,
    E_INTR_TYPE_RIO_MSG_OUT,
    E_INTR_TYPE_RIO_MSG_IN,
    E_INTR_TYPE_QE_LOW,
    E_INTR_TYPE_QE_HIGH,
    E_INTR_TYPE_QE_IO_PORTS,
    E_INTR_TYPE_QE_IRAM_ERR,
    E_INTR_TYPE_QE_MURAM_ERR,
    E_INTR_TYPE_QE_RTT,
    E_INTR_TYPE_QE_SDMA,
    E_INTR_TYPE_QE_VT,
    E_INTR_TYPE_QE_EXT_REQ
} e_interrupt_type;

/**************************************************************************//**
 @Description   TODO
*//***************************************************************************/
typedef enum ipc_search_type {
    E_IPC_SRC_AND_DEST_ADDRESS_SEARCH_TYPE,
    E_IPC_DEST_ADDRESS_SEARCH_TYPE,
    E_IPC_PORT_AND_QUEUE_SEARCH_TYPE
} e_ipc_search_type;


/**************************************************************************//**
 @Description   Chip Type and Revision Information Structure
*//***************************************************************************/
typedef struct t_chip_rev_info {
    char        chip_name[MAX_CHIP_NAME_LEN];
                /**< Chip name (e.g. "MPC8360E", "MPC8360") */
    uint16_t    rev_major;
                /**< Major chip revision */
    uint16_t    rev_minor;
                /**< Minor chip revision */
} t_chip_rev_info;

/**************************************************************************//**
 @Description   Descriptor memory-partition
*//***************************************************************************/
typedef struct platform_memory_info {
    int             mem_region_id;
    int             mem_partition_id;
    dma_addr_t      phys_base_addr;
    uintptr_t       virt_base_addr;
    uint64_t        size;
} t_platform_memory_info;

typedef struct t_platform_ops {
    fsl_handle_t h_platform;

    /* Platform initialization functions */
    int (*f_init_core)           (fsl_handle_t h_platform);
    int (*f_init_timer)          (fsl_handle_t h_platform);
    int (*f_init_intr_ctrl)      (fsl_handle_t h_platform);
    int (*f_init_soc)            (fsl_handle_t h_platform);  /**< For master partition only */
    int (*f_init_mem_partitions) (fsl_handle_t h_platform);
    int (*f_init_ipc)            (fsl_handle_t h_platform);
    int (*f_init_console)        (fsl_handle_t h_platform);
    int (*f_init_private)        (fsl_handle_t h_platform);

    /* Platform termination functions */
    int (*f_free_core)           (fsl_handle_t h_platform);
    int (*f_free_timer)          (fsl_handle_t h_platform);
    int (*f_free_intr_ctrl)      (fsl_handle_t h_platform);
    int (*f_free_soc)            (fsl_handle_t h_platform);  /**< For master partition only */
    int (*f_free_mem_partitions) (fsl_handle_t h_platform);
    int (*f_free_ipc)            (fsl_handle_t h_platform);
    int (*f_free_console)        (fsl_handle_t h_platform);
    int (*f_free_private)        (fsl_handle_t h_platform);

    /* Enable/disable functions */
    void    (*f_enable_cores)          (fsl_handle_t h_platform, uint64_t core_mask);
    void    (*f_enable_local_irq)      (fsl_handle_t h_platform);
    void    (*f_disable_local_irq)     (fsl_handle_t h_platform);
} t_platform_ops;


/**************************************************************************//**
 @Description   Callback Function Prototype for Link Events
*//***************************************************************************/
typedef void (t_platform_link_events_cb)(fsl_handle_t          h_controller,
                                         uint8_t           link_id,
                                         e_platform_event   event);

typedef int (t_tx_ipc_port_function)(fsl_handle_t   h_ipc_port,
                                           uint32_t   queue_id,
                                           uint8_t    *p_data,
                                           uint32_t   data_length);

/* this prototype is for rx polling operation */
typedef int (t_rx_ipc_port_function)(fsl_handle_t   h_ipc_port,
                                           uint32_t   queue_id);

typedef int (t_rx_conf_ipc_port_function)(fsl_handle_t h_ipc_port,
                                                uint32_t queue_id,
                                                uint8_t  *p_buffer);


#if defined(LS2100A) && defined(MC)
#include "arch/ls2100a/platform_mc_spec.h"
#elif defined(LS2100A) && defined(AIOP)
#include "arch/ls2100a/platform_aiop_spec.h"
#else
#error "Platform specific header file not defined"
#endif /* specific platform select */


/**************************************************************************//**
 @Function      platform_early_init

 @Description   Initializes the platform object according to the user's
                parameters and the board design and constraints.

 @Param[in]     p_EarlyParams - Pointer to platform early configuration parameters.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_early_init(struct platform_param *p_platform_params);

/**************************************************************************//**
 @Function      platform_init

 @Description   Creates the platform object and fills the platform operations
                structure, for later use by the system.

 @Param[in]     p_PlatformParam - Pointer to platform configuration parameters.
 @Param[out]    p_PlatformOps   - Pointer to platform operations structure,
                                  that should be filled by this routine.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_init(struct platform_param    *p_platform_param,
                        t_platform_ops             *p_platform_ops);

/**************************************************************************//**
 @Function      platform_free

 @Description   Releases the platform object and all its allocated resources.

 @Param[in]     h_Platform - Platform object handle.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_free(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_get_chip_rev_info

 @Description   Retrieves the revision information of the device.

 @Param[in]     h_Platform      - Platform object handle.
 @Param[out]    p_ChipRevInfo   - Returns the revision information structure.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_get_chip_rev_info(fsl_handle_t h_platform, t_chip_rev_info *p_chip_rev_info);

/**************************************************************************//**
 @Function      platform_get_memory_mapped_module_base

 @Description   Get the base address of a memory-mapped object.

 @Param[in]     h_Platform  - Platform object handle.
 @Param[in]     module      - Object type (usually a sub-module type)
 @Param[in]     id          - Object identifier

 @Return        Base address of a memory-mapped object.
*//***************************************************************************/
uintptr_t platform_get_memory_mapped_module_base(fsl_handle_t        h_platform,
                                             enum fsl_os_module     module,
                                             uint32_t        id,
                                             e_mapped_mem_type mapped_mem_type);

/**************************************************************************//**
 @Function      platform_get_core_clk

 @Description   Gets the core clock frequency of the device.

 @Param[in]     h_Platform - Platform object handle.

 @Return        The core clock frequency (Hz).
*//***************************************************************************/
uint32_t platform_get_core_clk(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_get_system_bus_clk

 @Description   Gets the system bus frequency of the device.

 @Param[in]     h_Platform - Platform object handle.

 @Return        The system bus frequency (Hz).
*//***************************************************************************/
uint32_t platform_get_system_bus_clk(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_get_ddr_clk

 @Description   Gets the local bus frequency of the device.

 @Param[in]     h_Platform - Platform object handle.

 @Return        The DDR clock frequency (Hz).
*//***************************************************************************/
uint32_t platform_get_ddr_clk(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_get_local_bus_clk

 @Description   Gets the local bus frequency of the device.

 @Param[in]     h_Platform - Platform object handle.

 @Return        The local bus frequency (Hz).
*//***************************************************************************/
uint32_t platform_get_local_bus_clk(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_get_controller_clk

 @Description   Gets the input clock frequency of a given controller object.

 @Param[in]     h_Platform  - Platform object handle.
 @Param[in]     module      - Controller object type (usually a sub-module type)
 @Param[in]     id          - Controller object identifier

 @Return        The input clock frequency of a given controller object (Hz).
*//***************************************************************************/
uint32_t platform_get_controller_clk(fsl_handle_t     h_platform,
                                   enum fsl_os_module  module,
                                   uint32_t     id);

/**************************************************************************//**
 @Function      platform_get_interrupt_id

 @Description   Gets the platform's interrupt ID of a given controller object.

 @Param[in]     h_Platform      - Platform object handle.
 @Param[in]     module          - Controller object type (usually a sub-module type)
 @Param[in]     id              - Controller object identifier
 @Param[in]     intrType        - Interrupt type selection
 @Param[in]     intrRelatedId   - Specific ID that may be related to some
                                  interrupt types

 @Return        The interrupt ID of a given controller object;
                Returns -'1' if not found or on any error.
*//***************************************************************************/
int platform_get_interrupt_id(fsl_handle_t        h_platform,
                            enum fsl_os_module     module,
                            uint32_t        id,
                            e_interrupt_type intr_type,
                            uint32_t        intr_related_id);

#if 0
/**************************************************************************//**
 @Function      platform_get_transaction_source_name

 @Description   Returns the name of a given bus transaction source.

 @Param[in]     h_Platform  - Platform object handle.
 @Param[in]     transSrc    - The requested transaction source.

 @Return        The name (string) of the given transaction source.
*//***************************************************************************/
char * platform_get_transaction_source_name(fsl_handle_t h_platform, e_trans_src trans_src);

/**************************************************************************//**
 @Function      platform_get_local_access_window_info

 @Description   Retrieves the base address and size of a local access window that
                is associated with a given controller object.

 @Param[in]     h_Platform  - Platform object handle.
 @Param[in]     module      - Controller object type (usually a sub-module type)
 @Param[in]     id          - Controller object identifier
 @Param[out]    p_BaseAddr  - Returns the physical base address of the local access window
 @Param[out]    p_Size      - Returns the size of the local access window

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_get_local_access_window_info(fsl_handle_t      h_platform,
                                          enum fsl_os_module   module,
                                          uint32_t      id,
                                          dma_addr_t *p_base_addr,
                                          uint64_t      *p_size);

/**************************************************************************//**
 @Function      platform_set_led

 @Description   Sets a selected LED on the board to on/off state.

 @Param[in]     h_Platform  - Platform object handle.
 @Param[in]     led         - LED color selection
 @Param[in]     ledOn       - '1' to turn the LED on; '0' to turn it off.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_set_led(fsl_handle_t h_platform, e_led_color led, int led_on);
#endif /* 0 */

/**************************************************************************//**
 @Function      platform_init_mac_for_mii_access

 @Description   Initialize MAC for MII access only.

 @Param[in]     h_Platform  - Platform object handle.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_init_mac_for_mii_access(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_free_mac_for_mii_access

 @Description   Free a MAC device that was previously initialized for
                MII access only.

 @Param[in]     h_Platform  - Platform object handle.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_free_mac_for_mii_access(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_connect_external_request

 @Description   Connects a specific module of the chip to external request signal.
                Usually used when firmware is using the external request as an
                interrupt signal for controlling the block without CPU involved.

@Param[in]      h_Platform      - Platform object handle.
@Param[in]      module          - Controller object type (usually a sub-module type)
@Param[in]      id              - Controller object identifier
@Param[in]      intrType        - Interrupt type selection
@Param[in]      intrRelatedId   - Specific ID that may be related to some
                                  interrupt types
@Param[in]      extReqNum       - External request number.


 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_connect_external_request(fsl_handle_t            h_platform,
                                        enum fsl_os_module         module,
                                        uint32_t            id,
                                        e_interrupt_type     intr_type,
                                        uint32_t            intr_related_id,
                                        uint8_t             ext_req_num);

/**************************************************************************//**
 @Function      platform_enable_console

 @Description   Enables the platform's console.

 @Param[in]     h_Platform - Platform object handle.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_enable_console(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_disable_console

 @Description   Disables the platform's console.

 @Param[in]     h_Platform - Platform object handle.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_disable_console(fsl_handle_t h_platform);

/**************************************************************************//**
 @Function      platform_set_serdes_loopback

 @Description   Sets SERDES loopback for the specified module.

 @Param[in]     h_Platform - Platform object handle.
 @Param[in]     module     - Controller object type (usually a sub-module type)
 @Param[in]     id         - Controller object identifier

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_set_serdes_loopback (fsl_handle_t    h_platform,
                                    enum fsl_os_module module,
                                    uint32_t id);

/**************************************************************************//**
 @Function      platform_clear_serdes_loopback

 @Description   Clears SERDES loopback for the specified module.

 @Param[in]     h_Platform - Platform object handle.
 @Param[in]     module     - Controller object type (usually a sub-module type)
 @Param[in]     id         - Controller object identifier

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
int platform_clear_serdes_loopback(fsl_handle_t    h_platform,
                                     enum fsl_os_module module,
                                     uint32_t    id);

/** @} */ /* end of platform_grp */

/**************************************************************************//**
 @Group         sys_platform_grp    System Platform Interface

 @Description   Bare-board system interface for platform-related operations.

 @{
*//***************************************************************************/
#include "inc/sys.h"
/**************************************************************************//**
 @Function      sys_get_memory_mapped_module_base

 @Description   Get the base address of a memory-mapped object.

 @Param[in]     module          - Object type (usually a sub-module type)
 @Param[in]     id              - Object identifier
 @Param[in]     mappedMemType   - Type of memory-mapped object, used to
                                  distinguish between different base types
                                  of the same object, e.g. PCI and PCI ATMU

 @Return        Base address of a memory-mapped object.
*//***************************************************************************/
static __inline__ uintptr_t sys_get_memory_mapped_module_base(enum fsl_os_module     module,
                                                              uint32_t               id,
                                                              e_mapped_mem_type mapped_mem_type)
{
    return platform_get_memory_mapped_module_base(sys_get_unique_handle(FSL_OS_MOD_SOC),
                                              module,
                                              id,
                                              mapped_mem_type);
}

/**************************************************************************//**
 @Function      sys_get_interrupt_id

 @Description   Gets the platform's interrupt ID of a given controller object.

 @Param[in]     module          - Controller object type (usually a sub-module type)
 @Param[in]     id              - Controller object identifier
 @Param[in]     intrType        - Interrupt type selection
 @Param[in]     intrRelatedId   - Specific ID that may be related to some
                                  interrupt types

 @Return        The interrupt ID of a given controller object;
                Returns -1 if not found or on any error.
*//***************************************************************************/
static __inline__ int sys_get_interrupt_id(enum fsl_os_module        module,
                                         uint32_t           id,
                                         e_interrupt_type    intr_type,
                                         uint32_t           intr_related_id)
{
    return platform_get_interrupt_id(sys_get_unique_handle(FSL_OS_MOD_SOC),
                                   module,
                                   id,
                                   intr_type,
                                   intr_related_id);
}

/**************************************************************************//**
 @Function      sys_get_controller_clk

 @Description   Gets the input clock frequency of a given controller object.

 @Param[in]     module  - Controller object type (usually a sub-module type)
 @Param[in]     id      - Controller object identifier

 @Return        The input clock frequency of a given controller object (Hz).
*//***************************************************************************/
static __inline__ uint32_t sys_get_controller_clk(enum fsl_os_module module, uint32_t id)
{
    return platform_get_controller_clk(sys_get_unique_handle(FSL_OS_MOD_SOC),
                                     module,
                                     id);
}

/**************************************************************************//**
 @Function      sys_get_core_clk

 @Description   Gets the core clock frequency of the device.

 @Return        The core clock frequency (Hz).
*//***************************************************************************/
static __inline__ uint32_t sys_get_core_clk(void)
{
    return platform_get_core_clk(sys_get_unique_handle(FSL_OS_MOD_SOC));
}

/**************************************************************************//**
 @Function      sys_get_system_bus_clk

 @Description   Gets the system bus frequency of the device.

 @Return        The system bus frequency (Hz).
*//***************************************************************************/
static __inline__ uint32_t sys_get_system_bus_clk(void)
{
    return platform_get_system_bus_clk(sys_get_unique_handle(FSL_OS_MOD_SOC));
}

#if 0
/**************************************************************************//**
 @Function      sys_get_chip_rev_info

 @Description   Retrieves the revision information of the device.

 @Param[out]    p_ChipRevInfo - Returns the revision information structure.

 @Return        E_OK on success; Error code otherwise.
*//***************************************************************************/
static __inline__ fsl_err_t sys_get_chip_rev_info(t_chip_rev_info *p_chip_rev_info)
{
    return platform_get_chip_rev_info(sys_get_unique_handle(FSL_OS_MOD_SOC),
                                   p_chip_rev_info);
}
#endif /* 0 */

/** @} */ /* end of sys_platform_grp */


#endif /* __FSL_PLATFORM_H */

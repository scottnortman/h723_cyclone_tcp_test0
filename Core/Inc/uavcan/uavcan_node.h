/**
 * @file uavcan_node.h
 * @brief UAVCAN Node Manager interface
 * 
 * This file defines the interface for UAVCAN node management including
 * node initialization, state management, and libudpard integration.
 */

#ifndef UAVCAN_NODE_H
#define UAVCAN_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"
#include "uavcan_transport.h"
#include "udpard.h"

/* Exported constants --------------------------------------------------------*/
#define UAVCAN_NODE_DEFAULT_MTU                 1408  /* Default MTU for UDP datagrams */
#define UAVCAN_NODE_TX_QUEUE_CAPACITY           32    /* Maximum TX queue size */
#define UAVCAN_NODE_MEMORY_POOL_SIZE            8192  /* Memory pool size in bytes */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief UAVCAN Node structure
 */
struct UavcanNode {
    /* Node identification */
    UdpardNodeID node_id;
    UavcanNodeState state;
    UavcanNodeHealth health;
    UavcanNodeMode mode;
    
    /* libudpard instances */
    struct UdpardTx tx_instance;
    
    /* Transport layer */
    UavcanTransport transport;
    
    /* Configuration */
    UavcanConfig config;
    
    /* Statistics and status */
    UavcanNodeStatus status;
    
    /* Memory management */
    struct UdpardMemoryResource memory_resource;
    uint8_t memory_pool[UAVCAN_NODE_MEMORY_POOL_SIZE];
    size_t memory_pool_offset;
    
    /* Synchronization */
    OsMutex node_mutex;
    
    /* State flags */
    bool initialized;
    bool started;
    
    /* Dynamic node ID allocation */
    bool dynamic_node_id_enabled;
    systime_t dynamic_node_id_start_time;
    uint32_t dynamic_node_id_attempts;
};

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Initialize UAVCAN node
 * @param node Pointer to node structure
 * @param interface Pointer to network interface
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeInit(UavcanNode* node, NetInterface* interface);

/**
 * @brief Deinitialize UAVCAN node
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeDeinit(UavcanNode* node);

/**
 * @brief Start UAVCAN node operations
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeStart(UavcanNode* node);

/**
 * @brief Stop UAVCAN node operations
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeStop(UavcanNode* node);

/**
 * @brief Set node ID
 * @param node Pointer to node structure
 * @param node_id Node ID to set (1-127, or 0 for unset)
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeSetNodeId(UavcanNode* node, UdpardNodeID node_id);

/**
 * @brief Get current node ID
 * @param node Pointer to node structure
 * @retval UdpardNodeID Current node ID (0 if unset)
 */
UdpardNodeID uavcanNodeGetNodeId(const UavcanNode* node);

/**
 * @brief Set node health status
 * @param node Pointer to node structure
 * @param health Health status to set
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeSetHealth(UavcanNode* node, UavcanNodeHealth health);

/**
 * @brief Set node mode
 * @param node Pointer to node structure
 * @param mode Mode to set
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeSetMode(UavcanNode* node, UavcanNodeMode mode);

/**
 * @brief Get node status
 * @param node Pointer to node structure
 * @param status Pointer to status structure to fill
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeGetStatus(const UavcanNode* node, UavcanNodeStatus* status);

/**
 * @brief Enable dynamic node ID allocation
 * @param node Pointer to node structure
 * @param enable True to enable, false to disable
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeEnableDynamicNodeId(UavcanNode* node, bool enable);

/**
 * @brief Process dynamic node ID allocation
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeProcessDynamicNodeId(UavcanNode* node);

/**
 * @brief Check if node is initialized
 * @param node Pointer to node structure
 * @retval bool True if initialized, false otherwise
 */
bool uavcanNodeIsInitialized(const UavcanNode* node);

/**
 * @brief Check if node is started
 * @param node Pointer to node structure
 * @retval bool True if started, false otherwise
 */
bool uavcanNodeIsStarted(const UavcanNode* node);

/**
 * @brief Update node uptime
 * @param node Pointer to node structure
 */
void uavcanNodeUpdateUptime(UavcanNode* node);

/**
 * @brief Memory allocator function for libudpard
 * @param user_reference User reference (UavcanNode pointer)
 * @param size Size to allocate
 * @retval void* Allocated memory or NULL on failure
 */
void* uavcanNodeMemoryAllocate(void* const user_reference, const size_t size);

/**
 * @brief Memory deallocator function for libudpard
 * @param user_reference User reference (UavcanNode pointer)
 * @param size Size of the memory block being freed
 * @param pointer Pointer to free
 */
void uavcanNodeMemoryFree(void* const user_reference, const size_t size, void* const pointer);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_NODE_H */
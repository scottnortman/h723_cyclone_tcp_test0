/**
 * @file uavcan_node.c
 * @brief UAVCAN Node Manager implementation
 * 
 * This file implements the UAVCAN node management functionality including
 * node initialization, state management, and libudpard integration.
 */

/* Includes ------------------------------------------------------------------*/
#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_transport.h"
#include "uavcan/uavcan_types.h"
#include "cmsis_os.h"

#include <string.h>
#include <stdlib.h>

/* Private defines -----------------------------------------------------------*/
#define UAVCAN_DYNAMIC_NODE_ID_TIMEOUT_MS       3000  /* 3 seconds timeout for dynamic allocation */
#define UAVCAN_DYNAMIC_NODE_ID_MAX_ATTEMPTS     10    /* Maximum attempts for dynamic allocation */

/* Private variables ---------------------------------------------------------*/
static systime_t node_start_time = 0;

/* Private function prototypes -----------------------------------------------*/
static UavcanError uavcanNodeInitializeMemory(UavcanNode* node);
static UavcanError uavcanNodeInitializeTransmission(UavcanNode* node);
static void uavcanNodeResetStatistics(UavcanNode* node);
static bool uavcanNodeValidateNodeId(UdpardNodeID node_id);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize UAVCAN node
 * @param node Pointer to node structure
 * @param interface Pointer to network interface
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeInit(UavcanNode* node, NetInterface* interface)
{
    UavcanError error;
    
    // Validate parameters
    if (node == NULL || interface == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Check if already initialized
    if (node->initialized) {
        return UAVCAN_ERROR_ALREADY_INITIALIZED;
    }
    
    // Initialize structure to zero
    memset(node, 0, sizeof(UavcanNode));
    
    // Set initial state
    node->state = UAVCAN_NODE_STATE_INITIALIZING;
    node->health = UAVCAN_NODE_HEALTH_NOMINAL;
    node->mode = UAVCAN_NODE_MODE_INITIALIZATION;
    node->node_id = UAVCAN_NODE_ID_UNSET;
    
    // Initialize default configuration
    node->config.node_id = UAVCAN_NODE_ID_UNSET;
    node->config.udp_port = UAVCAN_UDP_PORT;
    node->config.multicast_base = UAVCAN_SUBJECT_MULTICAST_BASE;
    node->config.heartbeat_interval_ms = UAVCAN_DEFAULT_HEARTBEAT_INTERVAL;
    node->config.debug_enabled = false;
    node->config.auto_start = false;
    
    // Create mutex for thread safety
    if (!osCreateMutex(&node->node_mutex)) {
        return UAVCAN_ERROR_INIT_FAILED;
    }
    
    // Initialize memory management
    error = uavcanNodeInitializeMemory(node);
    if (error != UAVCAN_ERROR_NONE) {
        osDeleteMutex(&node->node_mutex);
        return error;
    }
    
    // Initialize transport layer
    error = uavcanTransportInit(&node->transport, interface);
    if (error != UAVCAN_ERROR_NONE) {
        osDeleteMutex(&node->node_mutex);
        return error;
    }
    
    // Initialize transmission pipeline
    error = uavcanNodeInitializeTransmission(node);
    if (error != UAVCAN_ERROR_NONE) {
        uavcanTransportDeinit(&node->transport);
        osDeleteMutex(&node->node_mutex);
        return error;
    }
    
    // Reset statistics
    uavcanNodeResetStatistics(node);
    
    // Record start time
    if (node_start_time == 0) {
        node_start_time = osKernelSysTick();
    }
    
    // Mark as initialized
    node->initialized = true;
    node->state = UAVCAN_NODE_STATE_OFFLINE;
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Deinitialize UAVCAN node
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeDeinit(UavcanNode* node)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Stop node if running
    if (node->started) {
        uavcanNodeStop(node);
    }
    
    // Deinitialize transport
    uavcanTransportDeinit(&node->transport);
    
    // Delete mutex
    osDeleteMutex(&node->node_mutex);
    
    // Clear structure
    memset(node, 0, sizeof(UavcanNode));
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Start UAVCAN node operations
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeStart(UavcanNode* node)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    if (node->started) {
        return UAVCAN_ERROR_NONE; // Already started
    }
    
    osAcquireMutex(&node->node_mutex);
    
    // Check if we have a valid node ID or dynamic allocation is enabled
    if (node->node_id == UAVCAN_NODE_ID_UNSET && !node->dynamic_node_id_enabled) {
        osReleaseMutex(&node->node_mutex);
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Update state
    node->state = UAVCAN_NODE_STATE_OPERATIONAL;
    node->mode = UAVCAN_NODE_MODE_OPERATIONAL;
    node->started = true;
    
    // Reset statistics
    uavcanNodeResetStatistics(node);
    
    osReleaseMutex(&node->node_mutex);
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Stop UAVCAN node operations
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeStop(UavcanNode* node)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    if (!node->started) {
        return UAVCAN_ERROR_NONE; // Already stopped
    }
    
    osAcquireMutex(&node->node_mutex);
    
    // Update state
    node->state = UAVCAN_NODE_STATE_OFFLINE;
    node->mode = UAVCAN_NODE_MODE_OFFLINE;
    node->started = false;
    
    osReleaseMutex(&node->node_mutex);
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Set node ID
 * @param node Pointer to node structure
 * @param node_id Node ID to set (1-127, or 0 for unset)
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeSetNodeId(UavcanNode* node, UdpardNodeID node_id)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Validate node ID (0 is allowed for unset)
    if (node_id != UAVCAN_NODE_ID_UNSET && !uavcanNodeValidateNodeId(node_id)) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    osAcquireMutex(&node->node_mutex);
    
    node->node_id = node_id;
    node->config.node_id = node_id;
    
    // If we set a valid node ID, disable dynamic allocation
    if (node_id != UAVCAN_NODE_ID_UNSET) {
        node->dynamic_node_id_enabled = false;
    }
    
    osReleaseMutex(&node->node_mutex);
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get current node ID
 * @param node Pointer to node structure
 * @retval UdpardNodeID Current node ID (0 if unset)
 */
UdpardNodeID uavcanNodeGetNodeId(const UavcanNode* node)
{
    if (node == NULL || !node->initialized) {
        return UAVCAN_NODE_ID_UNSET;
    }
    
    return node->node_id;
}

/**
 * @brief Set node health status
 * @param node Pointer to node structure
 * @param health Health status to set
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeSetHealth(UavcanNode* node, UavcanNodeHealth health)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Validate health value
    if (health > UAVCAN_NODE_HEALTH_WARNING) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    osAcquireMutex(&node->node_mutex);
    node->health = health;
    osReleaseMutex(&node->node_mutex);
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Set node mode
 * @param node Pointer to node structure
 * @param mode Mode to set
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeSetMode(UavcanNode* node, UavcanNodeMode mode)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Validate mode value
    if (mode > UAVCAN_NODE_MODE_OFFLINE && mode != UAVCAN_NODE_MODE_OFFLINE) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    osAcquireMutex(&node->node_mutex);
    node->mode = mode;
    osReleaseMutex(&node->node_mutex);
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get node status
 * @param node Pointer to node structure
 * @param status Pointer to status structure to fill
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeGetStatus(const UavcanNode* node, UavcanNodeStatus* status)
{
    if (node == NULL || status == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Copy current status
    memcpy(status, &node->status, sizeof(UavcanNodeStatus));
    
    // Update dynamic fields
    status->state = node->state;
    status->node_id = node->node_id;
    status->health = node->health;
    status->mode = node->mode;
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Enable dynamic node ID allocation
 * @param node Pointer to node structure
 * @param enable True to enable, false to disable
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeEnableDynamicNodeId(UavcanNode* node, bool enable)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    osAcquireMutex(&node->node_mutex);
    
    node->dynamic_node_id_enabled = enable;
    
    if (enable) {
        // Reset dynamic allocation state
        node->dynamic_node_id_start_time = osKernelSysTick();
        node->dynamic_node_id_attempts = 0;
        
        // Clear current node ID if set
        if (node->node_id != UAVCAN_NODE_ID_UNSET) {
            node->node_id = UAVCAN_NODE_ID_UNSET;
            node->config.node_id = UAVCAN_NODE_ID_UNSET;
        }
    }
    
    osReleaseMutex(&node->node_mutex);
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Process dynamic node ID allocation
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanNodeProcessDynamicNodeId(UavcanNode* node)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!node->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    if (!node->dynamic_node_id_enabled) {
        return UAVCAN_ERROR_NONE; // Not enabled
    }
    
    if (node->node_id != UAVCAN_NODE_ID_UNSET) {
        return UAVCAN_ERROR_NONE; // Already allocated
    }
    
    osAcquireMutex(&node->node_mutex);
    
    systime_t current_time = osKernelSysTick();
    systime_t elapsed = current_time - node->dynamic_node_id_start_time;
    
    // Check timeout
    if (elapsed > UAVCAN_DYNAMIC_NODE_ID_TIMEOUT_MS) {
        node->dynamic_node_id_attempts++;
        
        if (node->dynamic_node_id_attempts >= UAVCAN_DYNAMIC_NODE_ID_MAX_ATTEMPTS) {
            // Give up on dynamic allocation
            node->dynamic_node_id_enabled = false;
            node->state = UAVCAN_NODE_STATE_ERROR;
            osReleaseMutex(&node->node_mutex);
            return UAVCAN_ERROR_TIMEOUT;
        }
        
        // Restart allocation process
        node->dynamic_node_id_start_time = current_time;
    }
    
    // TODO: Implement actual dynamic node ID allocation protocol
    // For now, this is a placeholder that would need to implement
    // the UAVCAN dynamic node ID allocation protocol
    
    osReleaseMutex(&node->node_mutex);
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Check if node is initialized
 * @param node Pointer to node structure
 * @retval bool True if initialized, false otherwise
 */
bool uavcanNodeIsInitialized(const UavcanNode* node)
{
    if (node == NULL) {
        return false;
    }
    
    return node->initialized;
}

/**
 * @brief Check if node is started
 * @param node Pointer to node structure
 * @retval bool True if started, false otherwise
 */
bool uavcanNodeIsStarted(const UavcanNode* node)
{
    if (node == NULL || !node->initialized) {
        return false;
    }
    
    return node->started;
}

/**
 * @brief Update node uptime
 * @param node Pointer to node structure
 */
void uavcanNodeUpdateUptime(UavcanNode* node)
{
    if (node == NULL || !node->initialized) {
        return;
    }
    
    systime_t current_time = osKernelSysTick();
    node->status.uptime_sec = (current_time - node_start_time) / 1000;
}

/**
 * @brief Memory allocator function for libudpard
 * @param user_reference User reference (UavcanNode pointer)
 * @param size Size to allocate
 * @retval void* Allocated memory or NULL on failure
 */
void* uavcanNodeMemoryAllocate(void* const user_reference, const size_t size)
{
    UavcanNode* node = (UavcanNode*)user_reference;
    
    if (node == NULL || size == 0) {
        return NULL;
    }
    
    // Simple bump allocator from memory pool
    if (node->memory_pool_offset + size > UAVCAN_NODE_MEMORY_POOL_SIZE) {
        return NULL; // Out of memory
    }
    
    void* ptr = &node->memory_pool[node->memory_pool_offset];
    node->memory_pool_offset += size;
    
    // Align to 4-byte boundary
    node->memory_pool_offset = (node->memory_pool_offset + 3) & ~3;
    
    return ptr;
}

/**
 * @brief Memory deallocator function for libudpard
 * @param user_reference User reference (UavcanNode pointer)
 * @param size Size of the memory block being freed
 * @param pointer Pointer to free
 */
void uavcanNodeMemoryFree(void* const user_reference, const size_t size, void* const pointer)
{
    // Simple bump allocator doesn't support individual frees
    // In a production system, you might want to use a more sophisticated allocator
    (void)user_reference;
    (void)size;
    (void)pointer;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Initialize memory management for the node
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
static UavcanError uavcanNodeInitializeMemory(UavcanNode* node)
{
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Initialize memory pool
    memset(node->memory_pool, 0, UAVCAN_NODE_MEMORY_POOL_SIZE);
    node->memory_pool_offset = 0;
    
    // Setup memory resource for libudpard
    node->memory_resource.user_reference = node;
    node->memory_resource.allocate = uavcanNodeMemoryAllocate;
    node->memory_resource.deallocate = uavcanNodeMemoryFree;
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Initialize transmission pipeline
 * @param node Pointer to node structure
 * @retval UavcanError Error code
 */
static UavcanError uavcanNodeInitializeTransmission(UavcanNode* node)
{
    int_fast8_t result;
    
    if (node == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Initialize TX pipeline
    result = udpardTxInit(&node->tx_instance,
                         &node->node_id,
                         UAVCAN_NODE_TX_QUEUE_CAPACITY,
                         node->memory_resource);
    
    if (result < 0) {
        return UAVCAN_ERROR_INIT_FAILED;
    }
    
    // Set default MTU
    node->tx_instance.mtu = UAVCAN_NODE_DEFAULT_MTU;
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Reset node statistics
 * @param node Pointer to node structure
 */
static void uavcanNodeResetStatistics(UavcanNode* node)
{
    if (node == NULL) {
        return;
    }
    
    memset(&node->status, 0, sizeof(UavcanNodeStatus));
    node->status.state = node->state;
    node->status.node_id = node->node_id;
    node->status.health = node->health;
    node->status.mode = node->mode;
}

/**
 * @brief Validate node ID
 * @param node_id Node ID to validate
 * @retval bool True if valid, false otherwise
 */
static bool uavcanNodeValidateNodeId(UdpardNodeID node_id)
{
    return UAVCAN_IS_VALID_NODE_ID(node_id);
}
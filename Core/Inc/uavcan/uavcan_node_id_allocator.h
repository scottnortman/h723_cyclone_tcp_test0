#ifndef UAVCAN_NODE_ID_ALLOCATOR_H
#define UAVCAN_NODE_ID_ALLOCATOR_H

#include "uavcan_common.h"
#include "uavcan_types.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Dynamic Node ID Allocation Constants
#define UAVCAN_DYNAMIC_NODE_ID_ALLOCATION_TIMEOUT_MS    10000  // 10 seconds
#define UAVCAN_DYNAMIC_NODE_ID_ALLOCATION_RETRY_COUNT   3
#define UAVCAN_DYNAMIC_NODE_ID_ALLOCATION_REQUEST_INTERVAL_MS 1000  // 1 second between requests
#define UAVCAN_DYNAMIC_NODE_ID_PREFERRED_MIN            1      // Preferred range start
#define UAVCAN_DYNAMIC_NODE_ID_PREFERRED_MAX            100    // Preferred range end

// Dynamic Node ID Allocation States
typedef enum {
    UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE = 0,
    UAVCAN_DYNAMIC_NODE_ID_STATE_REQUESTING,
    UAVCAN_DYNAMIC_NODE_ID_STATE_ALLOCATED,
    UAVCAN_DYNAMIC_NODE_ID_STATE_CONFLICT_DETECTED,
    UAVCAN_DYNAMIC_NODE_ID_STATE_FAILED
} UavcanDynamicNodeIdState;

// Dynamic Node ID Allocator Structure
typedef struct {
    UavcanDynamicNodeIdState state;
    uint8_t allocated_node_id;
    uint8_t preferred_node_id;
    uint32_t allocation_start_time;
    uint32_t last_request_time;
    uint8_t retry_count;
    bool allocation_in_progress;
    SemaphoreHandle_t state_mutex;
    
    // Callback function for allocation completion
    void (*allocation_complete_callback)(uint8_t node_id, bool success);
    
    // Network interface for sending allocation requests
    void* network_interface;  // Will be set to UDP transport when available
} UavcanDynamicNodeIdAllocator;

/**
 * @brief Initialize the dynamic node ID allocator
 * @param allocator Pointer to the allocator structure
 * @param preferred_node_id Preferred node ID (0 for any available)
 * @param callback Callback function called when allocation completes
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanDynamicNodeIdAllocatorInit(UavcanDynamicNodeIdAllocator* allocator,
                                         uint8_t preferred_node_id,
                                         void (*callback)(uint8_t node_id, bool success));

/**
 * @brief Start the dynamic node ID allocation process
 * @param allocator Pointer to the allocator structure
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanDynamicNodeIdAllocatorStart(UavcanDynamicNodeIdAllocator* allocator);

/**
 * @brief Stop the dynamic node ID allocation process
 * @param allocator Pointer to the allocator structure
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanDynamicNodeIdAllocatorStop(UavcanDynamicNodeIdAllocator* allocator);

/**
 * @brief Process dynamic node ID allocation (should be called periodically)
 * @param allocator Pointer to the allocator structure
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanDynamicNodeIdAllocatorProcess(UavcanDynamicNodeIdAllocator* allocator);

/**
 * @brief Handle incoming node ID allocation response
 * @param allocator Pointer to the allocator structure
 * @param response_node_id Node ID from the response
 * @param success Whether the allocation was successful
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanDynamicNodeIdAllocatorHandleResponse(UavcanDynamicNodeIdAllocator* allocator,
                                                   uint8_t response_node_id,
                                                   bool success);

/**
 * @brief Detect node ID conflict and handle it
 * @param allocator Pointer to the allocator structure
 * @param conflicting_node_id Node ID that has a conflict
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanDynamicNodeIdAllocatorDetectConflict(UavcanDynamicNodeIdAllocator* allocator,
                                                   uint8_t conflicting_node_id);

/**
 * @brief Get the current state of the allocator
 * @param allocator Pointer to the allocator structure
 * @return Current allocation state
 */
UavcanDynamicNodeIdState uavcanDynamicNodeIdAllocatorGetState(const UavcanDynamicNodeIdAllocator* allocator);

/**
 * @brief Get the allocated node ID
 * @param allocator Pointer to the allocator structure
 * @return Allocated node ID (0 if not allocated)
 */
uint8_t uavcanDynamicNodeIdAllocatorGetAllocatedId(const UavcanDynamicNodeIdAllocator* allocator);

/**
 * @brief Check if allocation is complete
 * @param allocator Pointer to the allocator structure
 * @return true if allocation is complete, false otherwise
 */
bool uavcanDynamicNodeIdAllocatorIsComplete(const UavcanDynamicNodeIdAllocator* allocator);

/**
 * @brief Reset the allocator to initial state
 * @param allocator Pointer to the allocator structure
 */
void uavcanDynamicNodeIdAllocatorReset(UavcanDynamicNodeIdAllocator* allocator);

/**
 * @brief Get a fallback node ID when dynamic allocation fails
 * @param allocator Pointer to the allocator structure
 * @return Fallback node ID
 */
uint8_t uavcanDynamicNodeIdAllocatorGetFallbackId(const UavcanDynamicNodeIdAllocator* allocator);

/**
 * @brief Validate if a node ID is available for allocation
 * @param node_id Node ID to validate
 * @return true if available, false otherwise
 */
bool uavcanDynamicNodeIdAllocatorIsIdAvailable(uint8_t node_id);

/**
 * @brief Get status string for the allocator
 * @param allocator Pointer to the allocator structure
 * @param buffer Buffer to store the status string
 * @param buffer_size Size of the buffer
 * @return Number of characters written to buffer
 */
size_t uavcanDynamicNodeIdAllocatorGetStatusString(const UavcanDynamicNodeIdAllocator* allocator,
                                                   char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_NODE_ID_ALLOCATOR_H
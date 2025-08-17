#include "uavcan/uavcan_node_id_allocator.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// Static variables for tracking allocated node IDs
static bool allocated_node_ids[UAVCAN_NODE_ID_MAX + 1] = {false};
static SemaphoreHandle_t allocation_table_mutex = NULL;

// Helper function to get current time in milliseconds
static uint32_t getCurrentTimeMs(void) {
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

// Helper function to find next available node ID
static uint8_t findNextAvailableNodeId(uint8_t preferred_id) {
    // First try the preferred ID if specified and valid
    if (preferred_id >= UAVCAN_NODE_ID_MIN && preferred_id <= UAVCAN_NODE_ID_MAX) {
        if (uavcanDynamicNodeIdAllocatorIsIdAvailable(preferred_id)) {
            return preferred_id;
        }
    }
    
    // Try preferred range first
    for (uint8_t id = UAVCAN_DYNAMIC_NODE_ID_PREFERRED_MIN; 
         id <= UAVCAN_DYNAMIC_NODE_ID_PREFERRED_MAX; id++) {
        if (uavcanDynamicNodeIdAllocatorIsIdAvailable(id)) {
            return id;
        }
    }
    
    // Try full range
    for (uint8_t id = UAVCAN_NODE_ID_MIN; id <= UAVCAN_NODE_ID_MAX; id++) {
        if (uavcanDynamicNodeIdAllocatorIsIdAvailable(id)) {
            return id;
        }
    }
    
    return UAVCAN_NODE_ID_UNSET; // No available ID found
}

/**
 * @brief Initialize the dynamic node ID allocator
 */
error_t uavcanDynamicNodeIdAllocatorInit(UavcanDynamicNodeIdAllocator* allocator,
                                         uint8_t preferred_node_id,
                                         void (*callback)(uint8_t node_id, bool success)) {
    if (allocator == NULL) {
        UAVCAN_ERROR_PRINT("Allocator is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize allocation table mutex if not already done
    if (allocation_table_mutex == NULL) {
        allocation_table_mutex = xSemaphoreCreateMutex();
        if (allocation_table_mutex == NULL) {
            UAVCAN_ERROR_PRINT("Failed to create allocation table mutex");
            return UAVCAN_ERROR_MEMORY_ALLOCATION;
        }
    }

    // Create state mutex
    allocator->state_mutex = xSemaphoreCreateMutex();
    if (allocator->state_mutex == NULL) {
        UAVCAN_ERROR_PRINT("Failed to create allocator state mutex");
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    // Initialize allocator state
    uavcanDynamicNodeIdAllocatorReset(allocator);
    allocator->preferred_node_id = preferred_node_id;
    allocator->allocation_complete_callback = callback;

    UAVCAN_INFO_PRINT("Dynamic node ID allocator initialized (preferred ID: %d)", preferred_node_id);
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Start the dynamic node ID allocation process
 */
error_t uavcanDynamicNodeIdAllocatorStart(UavcanDynamicNodeIdAllocator* allocator) {
    if (allocator == NULL) {
        UAVCAN_ERROR_PRINT("Allocator is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(allocator->state_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        UAVCAN_ERROR_PRINT("Failed to acquire allocator mutex");
        return UAVCAN_ERROR_TIMEOUT;
    }

    if (allocator->allocation_in_progress) {
        xSemaphoreGive(allocator->state_mutex);
        UAVCAN_WARN_PRINT("Allocation already in progress");
        return UAVCAN_ERROR_NONE;
    }

    // Reset state and start allocation
    allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_REQUESTING;
    allocator->allocation_in_progress = true;
    allocator->allocation_start_time = getCurrentTimeMs();
    allocator->last_request_time = 0;
    allocator->retry_count = 0;
    allocator->allocated_node_id = UAVCAN_NODE_ID_UNSET;

    xSemaphoreGive(allocator->state_mutex);

    UAVCAN_INFO_PRINT("Dynamic node ID allocation started");
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Stop the dynamic node ID allocation process
 */
error_t uavcanDynamicNodeIdAllocatorStop(UavcanDynamicNodeIdAllocator* allocator) {
    if (allocator == NULL) {
        UAVCAN_ERROR_PRINT("Allocator is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(allocator->state_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        UAVCAN_ERROR_PRINT("Failed to acquire allocator mutex");
        return UAVCAN_ERROR_TIMEOUT;
    }

    allocator->allocation_in_progress = false;
    allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE;

    xSemaphoreGive(allocator->state_mutex);

    UAVCAN_INFO_PRINT("Dynamic node ID allocation stopped");
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Process dynamic node ID allocation (should be called periodically)
 */
error_t uavcanDynamicNodeIdAllocatorProcess(UavcanDynamicNodeIdAllocator* allocator) {
    if (allocator == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(allocator->state_mutex, pdMS_TO_TICKS(10)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    if (!allocator->allocation_in_progress) {
        xSemaphoreGive(allocator->state_mutex);
        return UAVCAN_ERROR_NONE;
    }

    uint32_t current_time = getCurrentTimeMs();
    error_t result = UAVCAN_ERROR_NONE;

    switch (allocator->state) {
        case UAVCAN_DYNAMIC_NODE_ID_STATE_REQUESTING:
            // Check for timeout
            if (current_time - allocator->allocation_start_time > UAVCAN_DYNAMIC_NODE_ID_ALLOCATION_TIMEOUT_MS) {
                UAVCAN_WARN_PRINT("Dynamic node ID allocation timeout");
                allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_FAILED;
                break;
            }

            // Check if it's time to send another request
            if (current_time - allocator->last_request_time >= UAVCAN_DYNAMIC_NODE_ID_ALLOCATION_REQUEST_INTERVAL_MS) {
                // Find an available node ID
                uint8_t candidate_id = findNextAvailableNodeId(allocator->preferred_node_id);
                
                if (candidate_id == UAVCAN_NODE_ID_UNSET) {
                    UAVCAN_ERROR_PRINT("No available node IDs found");
                    allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_FAILED;
                    break;
                }

                // For now, simulate successful allocation since we don't have network layer yet
                // In a real implementation, this would send an allocation request over the network
                UAVCAN_INFO_PRINT("Simulating node ID allocation request for ID: %d", candidate_id);
                
                // Mark the ID as allocated in our local table
                if (xSemaphoreTake(allocation_table_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                    allocated_node_ids[candidate_id] = true;
                    xSemaphoreGive(allocation_table_mutex);
                }

                allocator->allocated_node_id = candidate_id;
                allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_ALLOCATED;
                allocator->allocation_in_progress = false;

                UAVCAN_INFO_PRINT("Node ID %d allocated successfully", candidate_id);

                // Call completion callback if provided
                if (allocator->allocation_complete_callback != NULL) {
                    allocator->allocation_complete_callback(candidate_id, true);
                }

                allocator->last_request_time = current_time;
            }
            break;

        case UAVCAN_DYNAMIC_NODE_ID_STATE_CONFLICT_DETECTED:
            UAVCAN_WARN_PRINT("Node ID conflict detected, restarting allocation");
            allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_REQUESTING;
            allocator->retry_count++;
            
            if (allocator->retry_count >= UAVCAN_DYNAMIC_NODE_ID_ALLOCATION_RETRY_COUNT) {
                UAVCAN_ERROR_PRINT("Maximum retry count reached");
                allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_FAILED;
            }
            break;

        case UAVCAN_DYNAMIC_NODE_ID_STATE_FAILED:
            allocator->allocation_in_progress = false;
            
            // Try fallback ID
            uint8_t fallback_id = uavcanDynamicNodeIdAllocatorGetFallbackId(allocator);
            if (fallback_id != UAVCAN_NODE_ID_UNSET) {
                UAVCAN_WARN_PRINT("Using fallback node ID: %d", fallback_id);
                allocator->allocated_node_id = fallback_id;
                allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_ALLOCATED;
                
                if (allocator->allocation_complete_callback != NULL) {
                    allocator->allocation_complete_callback(fallback_id, true);
                }
            } else {
                UAVCAN_ERROR_PRINT("Dynamic node ID allocation failed completely");
                if (allocator->allocation_complete_callback != NULL) {
                    allocator->allocation_complete_callback(UAVCAN_NODE_ID_UNSET, false);
                }
            }
            break;

        default:
            // IDLE or ALLOCATED states don't need processing
            break;
    }

    xSemaphoreGive(allocator->state_mutex);
    return result;
}

/**
 * @brief Handle incoming node ID allocation response
 */
error_t uavcanDynamicNodeIdAllocatorHandleResponse(UavcanDynamicNodeIdAllocator* allocator,
                                                   uint8_t response_node_id,
                                                   bool success) {
    if (allocator == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(allocator->state_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    if (allocator->state != UAVCAN_DYNAMIC_NODE_ID_STATE_REQUESTING) {
        xSemaphoreGive(allocator->state_mutex);
        return UAVCAN_ERROR_NONE; // Not expecting a response
    }

    if (success && uavcanIsValidNodeId(response_node_id)) {
        allocator->allocated_node_id = response_node_id;
        allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_ALLOCATED;
        allocator->allocation_in_progress = false;

        // Mark as allocated in global table
        if (xSemaphoreTake(allocation_table_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            allocated_node_ids[response_node_id] = true;
            xSemaphoreGive(allocation_table_mutex);
        }

        UAVCAN_INFO_PRINT("Node ID allocation response: ID %d allocated", response_node_id);

        if (allocator->allocation_complete_callback != NULL) {
            allocator->allocation_complete_callback(response_node_id, true);
        }
    } else {
        UAVCAN_WARN_PRINT("Node ID allocation response: allocation failed");
        allocator->retry_count++;
        
        if (allocator->retry_count >= UAVCAN_DYNAMIC_NODE_ID_ALLOCATION_RETRY_COUNT) {
            allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_FAILED;
        }
    }

    xSemaphoreGive(allocator->state_mutex);
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Detect node ID conflict and handle it
 */
error_t uavcanDynamicNodeIdAllocatorDetectConflict(UavcanDynamicNodeIdAllocator* allocator,
                                                   uint8_t conflicting_node_id) {
    if (allocator == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(allocator->state_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    if (allocator->allocated_node_id == conflicting_node_id) {
        UAVCAN_ERROR_PRINT("Node ID conflict detected for ID: %d", conflicting_node_id);
        allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_CONFLICT_DETECTED;
        allocator->allocated_node_id = UAVCAN_NODE_ID_UNSET;
        allocator->allocation_in_progress = true;
        
        // Mark as not allocated in global table
        if (xSemaphoreTake(allocation_table_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
            allocated_node_ids[conflicting_node_id] = false;
            xSemaphoreGive(allocation_table_mutex);
        }
    }

    xSemaphoreGive(allocator->state_mutex);
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get the current state of the allocator
 */
UavcanDynamicNodeIdState uavcanDynamicNodeIdAllocatorGetState(const UavcanDynamicNodeIdAllocator* allocator) {
    if (allocator == NULL) {
        return UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE;
    }
    return allocator->state;
}

/**
 * @brief Get the allocated node ID
 */
uint8_t uavcanDynamicNodeIdAllocatorGetAllocatedId(const UavcanDynamicNodeIdAllocator* allocator) {
    if (allocator == NULL) {
        return UAVCAN_NODE_ID_UNSET;
    }
    return allocator->allocated_node_id;
}

/**
 * @brief Check if allocation is complete
 */
bool uavcanDynamicNodeIdAllocatorIsComplete(const UavcanDynamicNodeIdAllocator* allocator) {
    if (allocator == NULL) {
        return false;
    }
    return (allocator->state == UAVCAN_DYNAMIC_NODE_ID_STATE_ALLOCATED);
}

/**
 * @brief Reset the allocator to initial state
 */
void uavcanDynamicNodeIdAllocatorReset(UavcanDynamicNodeIdAllocator* allocator) {
    if (allocator == NULL) {
        return;
    }

    allocator->state = UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE;
    allocator->allocated_node_id = UAVCAN_NODE_ID_UNSET;
    allocator->allocation_start_time = 0;
    allocator->last_request_time = 0;
    allocator->retry_count = 0;
    allocator->allocation_in_progress = false;
    allocator->network_interface = NULL;
}

/**
 * @brief Get a fallback node ID when dynamic allocation fails
 */
uint8_t uavcanDynamicNodeIdAllocatorGetFallbackId(const UavcanDynamicNodeIdAllocator* allocator) {
    // Use a simple fallback strategy: try a high node ID that's less likely to be used
    for (uint8_t id = UAVCAN_NODE_ID_MAX; id >= UAVCAN_NODE_ID_MAX - 10; id--) {
        if (uavcanDynamicNodeIdAllocatorIsIdAvailable(id)) {
            return id;
        }
    }
    
    // If no high IDs available, try any available ID
    return findNextAvailableNodeId(UAVCAN_NODE_ID_UNSET);
}

/**
 * @brief Validate if a node ID is available for allocation
 */
bool uavcanDynamicNodeIdAllocatorIsIdAvailable(uint8_t node_id) {
    if (!uavcanIsValidNodeId(node_id)) {
        return false;
    }

    bool available = true;
    if (allocation_table_mutex != NULL && 
        xSemaphoreTake(allocation_table_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        available = !allocated_node_ids[node_id];
        xSemaphoreGive(allocation_table_mutex);
    }

    return available;
}

/**
 * @brief Get status string for the allocator
 */
size_t uavcanDynamicNodeIdAllocatorGetStatusString(const UavcanDynamicNodeIdAllocator* allocator,
                                                   char* buffer, size_t buffer_size) {
    if (allocator == NULL || buffer == NULL || buffer_size == 0) {
        return 0;
    }

    const char* state_str;
    switch (allocator->state) {
        case UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE:
            state_str = "Idle";
            break;
        case UAVCAN_DYNAMIC_NODE_ID_STATE_REQUESTING:
            state_str = "Requesting";
            break;
        case UAVCAN_DYNAMIC_NODE_ID_STATE_ALLOCATED:
            state_str = "Allocated";
            break;
        case UAVCAN_DYNAMIC_NODE_ID_STATE_CONFLICT_DETECTED:
            state_str = "Conflict Detected";
            break;
        case UAVCAN_DYNAMIC_NODE_ID_STATE_FAILED:
            state_str = "Failed";
            break;
        default:
            state_str = "Unknown";
            break;
    }

    return snprintf(buffer, buffer_size,
                   "Dynamic Node ID Allocator Status:\n"
                   "State: %s\n"
                   "Allocated ID: %d\n"
                   "Preferred ID: %d\n"
                   "Retry Count: %d\n"
                   "In Progress: %s",
                   state_str,
                   allocator->allocated_node_id,
                   allocator->preferred_node_id,
                   allocator->retry_count,
                   allocator->allocation_in_progress ? "Yes" : "No");
}
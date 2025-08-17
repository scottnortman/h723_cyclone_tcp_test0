#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_node_id_allocator.h"
#include <string.h>
#include <stdio.h>

// Static variables for node management
static uint32_t node_start_time_ticks = 0;

// Dynamic allocation callback
static void dynamicAllocationCallback(uint8_t node_id, bool success) {
    if (success) {
        UAVCAN_INFO_PRINT("Dynamic node ID allocation completed: ID %d", node_id);
    } else {
        UAVCAN_ERROR_PRINT("Dynamic node ID allocation failed");
    }
}

/**
 * @brief Initialize a UAVCAN node context
 */
error_t uavcanNodeInit(UavcanNodeContext* ctx, uint8_t node_id) {
    // Validate input parameters
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Validate node ID if not using dynamic allocation
    if (node_id != UAVCAN_NODE_ID_UNSET) {
        error_t validation_result = uavcanNodeValidateConfig(node_id);
        if (UAVCAN_FAILED(validation_result)) {
            UAVCAN_ERROR_PRINT("Invalid node ID: %d", node_id);
            return validation_result;
        }
    }

    // Reset the context to ensure clean state
    uavcanNodeReset(ctx);

    // Initialize node context
    ctx->node_id = node_id;
    ctx->health = UAVCAN_NODE_HEALTH_NOMINAL;
    ctx->mode = UAVCAN_NODE_MODE_INITIALIZATION;
    ctx->uptime_sec = 0;
    ctx->dynamic_node_id_allocator = NULL; // Will be set up in dynamic allocation implementation
    ctx->initialized = false; // Will be set to true after successful start

    // Record initialization time
    node_start_time_ticks = xTaskGetTickCount();

    UAVCAN_INFO_PRINT("Node initialized with ID: %d", node_id);
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Start the UAVCAN node operations
 */
error_t uavcanNodeStart(UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (ctx->initialized) {
        UAVCAN_WARN_PRINT("Node already started");
        return UAVCAN_ERROR_NONE;
    }

    // Set node to operational mode
    ctx->mode = UAVCAN_NODE_MODE_OPERATIONAL;
    ctx->initialized = true;

    UAVCAN_INFO_PRINT("Node started successfully (ID: %d)", ctx->node_id);
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Stop the UAVCAN node operations
 */
error_t uavcanNodeStop(UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!ctx->initialized) {
        UAVCAN_WARN_PRINT("Node not started");
        return UAVCAN_ERROR_NONE;
    }

    // Set node to offline mode
    ctx->mode = UAVCAN_NODE_MODE_OFFLINE;
    ctx->initialized = false;

    UAVCAN_INFO_PRINT("Node stopped (ID: %d)", ctx->node_id);
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get the current health status of the node
 */
UavcanNodeHealth uavcanNodeGetHealth(const UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_NODE_HEALTH_WARNING;
    }
    return ctx->health;
}

/**
 * @brief Set the health status of the node
 */
error_t uavcanNodeSetHealth(UavcanNodeContext* ctx, UavcanNodeHealth health) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Validate health status
    if (health > UAVCAN_NODE_HEALTH_WARNING) {
        UAVCAN_ERROR_PRINT("Invalid health status: %d", health);
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    UavcanNodeHealth old_health = ctx->health;
    ctx->health = health;

    if (old_health != health) {
        UAVCAN_INFO_PRINT("Node health changed from %s to %s", 
                          uavcanNodeHealthToString(old_health),
                          uavcanNodeHealthToString(health));
    }

    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get the current mode of the node
 */
UavcanNodeMode uavcanNodeGetMode(const UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_NODE_MODE_OFFLINE;
    }
    return ctx->mode;
}

/**
 * @brief Set the mode of the node
 */
error_t uavcanNodeSetMode(UavcanNodeContext* ctx, UavcanNodeMode mode) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Validate mode (check for valid enum values)
    if (mode != UAVCAN_NODE_MODE_OPERATIONAL &&
        mode != UAVCAN_NODE_MODE_INITIALIZATION &&
        mode != UAVCAN_NODE_MODE_MAINTENANCE &&
        mode != UAVCAN_NODE_MODE_SOFTWARE_UPDATE &&
        mode != UAVCAN_NODE_MODE_OFFLINE) {
        UAVCAN_ERROR_PRINT("Invalid node mode: %d", mode);
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    UavcanNodeMode old_mode = ctx->mode;
    ctx->mode = mode;

    if (old_mode != mode) {
        UAVCAN_INFO_PRINT("Node mode changed from %s to %s", 
                          uavcanNodeModeToString(old_mode),
                          uavcanNodeModeToString(mode));
    }

    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get the node ID
 */
uint8_t uavcanNodeGetId(const UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_NODE_ID_UNSET;
    }
    return ctx->node_id;
}

/**
 * @brief Set the node ID
 */
error_t uavcanNodeSetId(UavcanNodeContext* ctx, uint8_t node_id) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Validate node ID if not using dynamic allocation
    if (node_id != UAVCAN_NODE_ID_UNSET) {
        error_t validation_result = uavcanNodeValidateConfig(node_id);
        if (UAVCAN_FAILED(validation_result)) {
            UAVCAN_ERROR_PRINT("Invalid node ID: %d", node_id);
            return validation_result;
        }
    }

    uint8_t old_id = ctx->node_id;
    ctx->node_id = node_id;

    if (old_id != node_id) {
        UAVCAN_INFO_PRINT("Node ID changed from %d to %d", old_id, node_id);
    }

    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Check if the node is initialized
 */
bool uavcanNodeIsInitialized(const UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        return false;
    }
    return ctx->initialized;
}

/**
 * @brief Get the node uptime in seconds
 */
uint32_t uavcanNodeGetUptime(const UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        return 0;
    }
    return ctx->uptime_sec;
}

/**
 * @brief Update the node uptime (should be called periodically)
 */
void uavcanNodeUpdateUptime(UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        return;
    }

    // Calculate uptime based on FreeRTOS tick count
    uint32_t current_ticks = xTaskGetTickCount();
    uint32_t elapsed_ticks = current_ticks - node_start_time_ticks;
    ctx->uptime_sec = elapsed_ticks / configTICK_RATE_HZ;
}

/**
 * @brief Validate node configuration parameters
 */
error_t uavcanNodeValidateConfig(uint8_t node_id) {
    // Node ID 0 is reserved for dynamic allocation
    if (node_id == UAVCAN_NODE_ID_UNSET) {
        return UAVCAN_ERROR_NONE; // Dynamic allocation is valid
    }

    // Check if node ID is within valid range
    if (!uavcanIsValidNodeId(node_id)) {
        UAVCAN_ERROR_PRINT("Node ID %d is out of valid range (%d-%d)", 
                          node_id, UAVCAN_NODE_ID_MIN, UAVCAN_NODE_ID_MAX);
        return UAVCAN_ERROR_INVALID_CONFIG;
    }

    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Reset the node context to default values
 */
void uavcanNodeReset(UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        return;
    }

    // Clear the entire structure
    memset(ctx, 0, sizeof(UavcanNodeContext));

    // Set default values
    ctx->node_id = UAVCAN_NODE_ID_UNSET;
    ctx->health = UAVCAN_NODE_HEALTH_NOMINAL;
    ctx->mode = UAVCAN_NODE_MODE_OFFLINE;
    ctx->uptime_sec = 0;
    ctx->initialized = false;
    ctx->dynamic_node_id_allocator = NULL;
}

/**
 * @brief Get node status information as a formatted string
 */
size_t uavcanNodeGetStatusString(const UavcanNodeContext* ctx, char* buffer, size_t buffer_size) {
    if (ctx == NULL || buffer == NULL || buffer_size == 0) {
        return 0;
    }

    // Update uptime before generating status
    uavcanNodeUpdateUptime((UavcanNodeContext*)ctx);

    return snprintf(buffer, buffer_size,
                   "Node ID: %d\n"
                   "Status: %s\n"
                   "Health: %s\n"
                   "Mode: %s\n"
                   "Uptime: %lu seconds\n"
                   "Initialized: %s",
                   ctx->node_id,
                   ctx->initialized ? "Running" : "Stopped",
                   uavcanNodeHealthToString(ctx->health),
                   uavcanNodeModeToString(ctx->mode),
                   (unsigned long)ctx->uptime_sec,
                   ctx->initialized ? "Yes" : "No");
}

/**
 * @brief Initialize dynamic node ID allocation for the node
 */
error_t uavcanNodeInitDynamicAllocation(UavcanNodeContext* ctx, uint8_t preferred_node_id) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Allocate memory for the dynamic node ID allocator
    ctx->dynamic_node_id_allocator = UAVCAN_MALLOC(sizeof(UavcanDynamicNodeIdAllocator));
    if (ctx->dynamic_node_id_allocator == NULL) {
        UAVCAN_ERROR_PRINT("Failed to allocate memory for dynamic node ID allocator");
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    // Initialize the allocator
    error_t result = uavcanDynamicNodeIdAllocatorInit(
        (UavcanDynamicNodeIdAllocator*)ctx->dynamic_node_id_allocator,
        preferred_node_id,
        dynamicAllocationCallback
    );

    if (UAVCAN_FAILED(result)) {
        UAVCAN_FREE(ctx->dynamic_node_id_allocator);
        ctx->dynamic_node_id_allocator = NULL;
        UAVCAN_ERROR_PRINT("Failed to initialize dynamic node ID allocator");
        return result;
    }

    UAVCAN_INFO_PRINT("Dynamic node ID allocation initialized (preferred ID: %d)", preferred_node_id);
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Start dynamic node ID allocation process
 */
error_t uavcanNodeStartDynamicAllocation(UavcanNodeContext* ctx) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Node context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (ctx->dynamic_node_id_allocator == NULL) {
        UAVCAN_ERROR_PRINT("Dynamic node ID allocator not initialized");
        return UAVCAN_ERROR_INIT_FAILED;
    }

    error_t result = uavcanDynamicNodeIdAllocatorStart(
        (UavcanDynamicNodeIdAllocator*)ctx->dynamic_node_id_allocator
    );

    if (UAVCAN_SUCCEEDED(result)) {
        UAVCAN_INFO_PRINT("Dynamic node ID allocation started");
    }

    return result;
}

/**
 * @brief Process dynamic node ID allocation (should be called periodically)
 */
error_t uavcanNodeProcessDynamicAllocation(UavcanNodeContext* ctx) {
    if (ctx == NULL || ctx->dynamic_node_id_allocator == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    error_t result = uavcanDynamicNodeIdAllocatorProcess(
        (UavcanDynamicNodeIdAllocator*)ctx->dynamic_node_id_allocator
    );

    // Check if allocation is complete and update node ID
    if (uavcanDynamicNodeIdAllocatorIsComplete(
        (UavcanDynamicNodeIdAllocator*)ctx->dynamic_node_id_allocator)) {
        
        uint8_t allocated_id = uavcanDynamicNodeIdAllocatorGetAllocatedId(
            (UavcanDynamicNodeIdAllocator*)ctx->dynamic_node_id_allocator
        );
        
        if (allocated_id != UAVCAN_NODE_ID_UNSET && ctx->node_id == UAVCAN_NODE_ID_UNSET) {
            ctx->node_id = allocated_id;
            UAVCAN_INFO_PRINT("Node ID updated to dynamically allocated ID: %d", allocated_id);
        }
    }

    return result;
}

/**
 * @brief Check if dynamic node ID allocation is complete
 */
bool uavcanNodeIsDynamicAllocationComplete(const UavcanNodeContext* ctx) {
    if (ctx == NULL || ctx->dynamic_node_id_allocator == NULL) {
        return false;
    }

    return uavcanDynamicNodeIdAllocatorIsComplete(
        (UavcanDynamicNodeIdAllocator*)ctx->dynamic_node_id_allocator
    );
}

/**
 * @brief Get the dynamically allocated node ID
 */
uint8_t uavcanNodeGetDynamicAllocatedId(const UavcanNodeContext* ctx) {
    if (ctx == NULL || ctx->dynamic_node_id_allocator == NULL) {
        return UAVCAN_NODE_ID_UNSET;
    }

    return uavcanDynamicNodeIdAllocatorGetAllocatedId(
        (UavcanDynamicNodeIdAllocator*)ctx->dynamic_node_id_allocator
    );
}
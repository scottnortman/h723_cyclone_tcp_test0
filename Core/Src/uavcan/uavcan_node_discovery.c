#include "uavcan/uavcan_node_discovery.h"
#include "uavcan/uavcan_common.h"
#include <string.h>
#include <stdio.h>

// Default node timeout (5 seconds)
#define UAVCAN_NODE_TIMEOUT_MS          5000

/**
 * @brief Get current timestamp in microseconds
 * @return uint64_t Current timestamp
 */
static uint64_t getCurrentTimestampUsec(void) {
    // Use FreeRTOS tick count converted to microseconds
    uint32_t ticks = xTaskGetTickCount();
    return (uint64_t)ticks * (1000000ULL / configTICK_RATE_HZ);
}

/**
 * @brief Find node entry by ID
 * @param discovery Pointer to discovery context
 * @param node_id Node ID to find
 * @return UavcanDiscoveredNode* Pointer to node entry or NULL if not found
 */
static UavcanDiscoveredNode* findNodeById(UavcanNodeDiscoveryContext* discovery, uint8_t node_id) {
    for (uint32_t i = 0; i < UAVCAN_MAX_DISCOVERED_NODES; i++) {
        if (discovery->nodes[i].is_active && discovery->nodes[i].node_id == node_id) {
            return &discovery->nodes[i];
        }
    }
    return NULL;
}

/**
 * @brief Find empty node slot
 * @param discovery Pointer to discovery context
 * @return UavcanDiscoveredNode* Pointer to empty slot or NULL if full
 */
static UavcanDiscoveredNode* findEmptySlot(UavcanNodeDiscoveryContext* discovery) {
    for (uint32_t i = 0; i < UAVCAN_MAX_DISCOVERED_NODES; i++) {
        if (!discovery->nodes[i].is_active) {
            return &discovery->nodes[i];
        }
    }
    return NULL;
}

error_t uavcanNodeDiscoveryInit(UavcanNodeDiscoveryContext* discovery) {
    if (discovery == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize discovery context
    memset(discovery, 0, sizeof(UavcanNodeDiscoveryContext));
    
    // Create mutex for thread safety
    discovery->mutex = xSemaphoreCreateMutex();
    if (discovery->mutex == NULL) {
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    discovery->node_count = 0;
    discovery->last_discovery_time = getCurrentTimestampUsec();

    return UAVCAN_ERROR_NONE;
}

error_t uavcanNodeDiscoveryUpdateFromHeartbeat(UavcanNodeDiscoveryContext* discovery,
                                              uint8_t node_id,
                                              UavcanNodeHealth health,
                                              UavcanNodeMode mode,
                                              uint32_t uptime) {
    if (discovery == NULL || !uavcanIsValidNodeId(node_id)) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(discovery->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    uint64_t current_time = getCurrentTimestampUsec();
    UavcanDiscoveredNode* node = findNodeById(discovery, node_id);

    if (node == NULL) {
        // New node, find empty slot
        node = findEmptySlot(discovery);
        if (node == NULL) {
            xSemaphoreGive(discovery->mutex);
            return UAVCAN_ERROR_QUEUE_FULL; // No more slots available
        }

        // Initialize new node entry
        memset(node, 0, sizeof(UavcanDiscoveredNode));
        node->node_id = node_id;
        node->is_active = true;
        discovery->node_count++;
        
        // Set default name
        snprintf(node->name, sizeof(node->name), "Node_%d", node_id);
    }

    // Update node information
    node->health = health;
    node->mode = mode;
    node->uptime_sec = uptime;
    node->last_seen_time = current_time;

    discovery->last_discovery_time = current_time;

    xSemaphoreGive(discovery->mutex);
    return UAVCAN_ERROR_NONE;
}

size_t uavcanNodeDiscoveryGetNodesString(const UavcanNodeDiscoveryContext* discovery,
                                        char* buffer, size_t buffer_size) {
    if (discovery == NULL || buffer == NULL || buffer_size == 0) {
        return 0;
    }

    if (xSemaphoreTake(discovery->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return snprintf(buffer, buffer_size, "Node discovery unavailable (mutex timeout)\r\n");
    }

    size_t written = 0;
    uint64_t current_time = getCurrentTimestampUsec();

    // Header
    written = snprintf(buffer, buffer_size,
        "Discovered UAVCAN Nodes (%lu active):\r\n"
        "  Node ID | Health    | Mode         | Uptime   | Last Seen | Name\r\n"
        "  --------|-----------|--------------|----------|-----------|----------\r\n",
        (unsigned long)discovery->node_count);

    // List active nodes
    bool found_any = false;
    for (uint32_t i = 0; i < UAVCAN_MAX_DISCOVERED_NODES && written < buffer_size - 100; i++) {
        const UavcanDiscoveredNode* node = &discovery->nodes[i];
        
        if (!node->is_active) {
            continue;
        }

        found_any = true;
        
        // Calculate time since last seen
        uint64_t time_diff_usec = current_time - node->last_seen_time;
        uint32_t time_diff_sec = (uint32_t)(time_diff_usec / 1000000ULL);
        
        char temp_buffer[150];
        snprintf(temp_buffer, sizeof(temp_buffer),
            "  %7d | %-9s | %-12s | %6lus | %7lus | %s\r\n",
            node->node_id,
            uavcanNodeHealthToString(node->health),
            uavcanNodeModeToString(node->mode),
            (unsigned long)node->uptime_sec,
            (unsigned long)time_diff_sec,
            node->name);
        
        if (written + strlen(temp_buffer) < buffer_size) {
            strcat(buffer, temp_buffer);
            written += strlen(temp_buffer);
        }
    }

    if (!found_any) {
        if (written + 30 < buffer_size) {
            strcat(buffer, "  No active nodes discovered\r\n");
            written += 30;
        }
    }

    xSemaphoreGive(discovery->mutex);
    return written;
}

uint32_t uavcanNodeDiscoveryGetActiveCount(const UavcanNodeDiscoveryContext* discovery) {
    if (discovery == NULL) {
        return 0;
    }

    if (xSemaphoreTake(discovery->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return 0;
    }

    uint32_t count = 0;
    uint64_t current_time = getCurrentTimestampUsec();

    // Count nodes that are still active (within timeout)
    for (uint32_t i = 0; i < UAVCAN_MAX_DISCOVERED_NODES; i++) {
        const UavcanDiscoveredNode* node = &discovery->nodes[i];
        
        if (!node->is_active) {
            continue;
        }

        uint64_t time_diff_usec = current_time - node->last_seen_time;
        uint32_t time_diff_ms = (uint32_t)(time_diff_usec / 1000ULL);
        
        if (time_diff_ms <= UAVCAN_NODE_TIMEOUT_MS) {
            count++;
        }
    }

    xSemaphoreGive(discovery->mutex);
    return count;
}

error_t uavcanNodeDiscoveryClear(UavcanNodeDiscoveryContext* discovery) {
    if (discovery == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(discovery->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Clear all nodes
    memset(discovery->nodes, 0, sizeof(discovery->nodes));
    discovery->node_count = 0;
    discovery->last_discovery_time = getCurrentTimestampUsec();

    xSemaphoreGive(discovery->mutex);
    return UAVCAN_ERROR_NONE;
}

bool uavcanNodeDiscoveryIsNodeActive(const UavcanNodeDiscoveryContext* discovery,
                                    uint8_t node_id, uint32_t timeout_ms) {
    if (discovery == NULL || !uavcanIsValidNodeId(node_id)) {
        return false;
    }

    if (xSemaphoreTake(discovery->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    bool is_active = false;
    uint64_t current_time = getCurrentTimestampUsec();
    
    const UavcanDiscoveredNode* node = findNodeById((UavcanNodeDiscoveryContext*)discovery, node_id);
    if (node != NULL) {
        uint64_t time_diff_usec = current_time - node->last_seen_time;
        uint32_t time_diff_ms = (uint32_t)(time_diff_usec / 1000ULL);
        is_active = (time_diff_ms <= timeout_ms);
    }

    xSemaphoreGive(discovery->mutex);
    return is_active;
}
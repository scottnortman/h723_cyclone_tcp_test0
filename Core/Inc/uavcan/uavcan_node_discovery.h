#ifndef UAVCAN_NODE_DISCOVERY_H
#define UAVCAN_NODE_DISCOVERY_H

#include "uavcan_types.h"
#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Maximum number of nodes to track
#define UAVCAN_MAX_DISCOVERED_NODES     32

// Node discovery entry
typedef struct {
    uint8_t node_id;
    UavcanNodeHealth health;
    UavcanNodeMode mode;
    uint64_t last_seen_time;
    uint32_t uptime_sec;
    char name[UAVCAN_MAX_NODE_NAME_LENGTH + 1];
    bool is_active;
} UavcanDiscoveredNode;

// Node discovery context
typedef struct {
    UavcanDiscoveredNode nodes[UAVCAN_MAX_DISCOVERED_NODES];
    uint32_t node_count;
    SemaphoreHandle_t mutex;
    uint64_t last_discovery_time;
} UavcanNodeDiscoveryContext;

/**
 * @brief Initialize node discovery system
 * @param discovery Pointer to discovery context
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanNodeDiscoveryInit(UavcanNodeDiscoveryContext* discovery);

/**
 * @brief Update node information from heartbeat message
 * @param discovery Pointer to discovery context
 * @param node_id Node ID from heartbeat
 * @param health Node health from heartbeat
 * @param mode Node mode from heartbeat
 * @param uptime Node uptime from heartbeat
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanNodeDiscoveryUpdateFromHeartbeat(UavcanNodeDiscoveryContext* discovery,
                                              uint8_t node_id,
                                              UavcanNodeHealth health,
                                              UavcanNodeMode mode,
                                              uint32_t uptime);

/**
 * @brief Get list of discovered nodes as formatted string
 * @param discovery Pointer to discovery context
 * @param buffer Buffer to store the list
 * @param buffer_size Size of the buffer
 * @return size_t Number of characters written
 */
size_t uavcanNodeDiscoveryGetNodesString(const UavcanNodeDiscoveryContext* discovery,
                                        char* buffer, size_t buffer_size);

/**
 * @brief Get count of active discovered nodes
 * @param discovery Pointer to discovery context
 * @return uint32_t Number of active nodes
 */
uint32_t uavcanNodeDiscoveryGetActiveCount(const UavcanNodeDiscoveryContext* discovery);

/**
 * @brief Clear all discovered nodes
 * @param discovery Pointer to discovery context
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanNodeDiscoveryClear(UavcanNodeDiscoveryContext* discovery);

/**
 * @brief Check if a node is recently active (within timeout)
 * @param discovery Pointer to discovery context
 * @param node_id Node ID to check
 * @param timeout_ms Timeout in milliseconds
 * @return bool true if node is active, false otherwise
 */
bool uavcanNodeDiscoveryIsNodeActive(const UavcanNodeDiscoveryContext* discovery,
                                    uint8_t node_id, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_NODE_DISCOVERY_H
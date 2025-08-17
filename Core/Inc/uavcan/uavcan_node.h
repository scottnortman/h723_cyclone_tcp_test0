#ifndef UAVCAN_NODE_H
#define UAVCAN_NODE_H

#include "uavcan_common.h"
#include "uavcan_types.h"
#include "uavcan_node_id_allocator.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize a UAVCAN node context
 * @param ctx Pointer to the node context structure
 * @param node_id Node ID to use (0 for dynamic allocation)
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeInit(UavcanNodeContext* ctx, uint8_t node_id);

/**
 * @brief Start the UAVCAN node operations
 * @param ctx Pointer to the initialized node context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeStart(UavcanNodeContext* ctx);

/**
 * @brief Stop the UAVCAN node operations
 * @param ctx Pointer to the node context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeStop(UavcanNodeContext* ctx);

/**
 * @brief Get the current health status of the node
 * @param ctx Pointer to the node context
 * @return Current node health status
 */
UavcanNodeHealth uavcanNodeGetHealth(const UavcanNodeContext* ctx);

/**
 * @brief Set the health status of the node
 * @param ctx Pointer to the node context
 * @param health New health status to set
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeSetHealth(UavcanNodeContext* ctx, UavcanNodeHealth health);

/**
 * @brief Get the current mode of the node
 * @param ctx Pointer to the node context
 * @return Current node mode
 */
UavcanNodeMode uavcanNodeGetMode(const UavcanNodeContext* ctx);

/**
 * @brief Set the mode of the node
 * @param ctx Pointer to the node context
 * @param mode New mode to set
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeSetMode(UavcanNodeContext* ctx, UavcanNodeMode mode);

/**
 * @brief Get the node ID
 * @param ctx Pointer to the node context
 * @return Current node ID (0 if using dynamic allocation)
 */
uint8_t uavcanNodeGetId(const UavcanNodeContext* ctx);

/**
 * @brief Set the node ID
 * @param ctx Pointer to the node context
 * @param node_id New node ID to set (must be valid)
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeSetId(UavcanNodeContext* ctx, uint8_t node_id);

/**
 * @brief Check if the node is initialized
 * @param ctx Pointer to the node context
 * @return true if initialized, false otherwise
 */
bool uavcanNodeIsInitialized(const UavcanNodeContext* ctx);

/**
 * @brief Get the node uptime in seconds
 * @param ctx Pointer to the node context
 * @return Node uptime in seconds
 */
uint32_t uavcanNodeGetUptime(const UavcanNodeContext* ctx);

/**
 * @brief Update the node uptime (should be called periodically)
 * @param ctx Pointer to the node context
 */
void uavcanNodeUpdateUptime(UavcanNodeContext* ctx);

/**
 * @brief Validate node configuration parameters
 * @param node_id Node ID to validate
 * @return UAVCAN_ERROR_NONE if valid, error code otherwise
 */
error_t uavcanNodeValidateConfig(uint8_t node_id);

/**
 * @brief Reset the node context to default values
 * @param ctx Pointer to the node context
 */
void uavcanNodeReset(UavcanNodeContext* ctx);

/**
 * @brief Get node status information as a formatted string
 * @param ctx Pointer to the node context
 * @param buffer Buffer to store the status string
 * @param buffer_size Size of the buffer
 * @return Number of characters written to buffer
 */
size_t uavcanNodeGetStatusString(const UavcanNodeContext* ctx, char* buffer, size_t buffer_size);

/**
 * @brief Initialize dynamic node ID allocation for the node
 * @param ctx Pointer to the node context
 * @param preferred_node_id Preferred node ID (0 for any available)
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeInitDynamicAllocation(UavcanNodeContext* ctx, uint8_t preferred_node_id);

/**
 * @brief Start dynamic node ID allocation process
 * @param ctx Pointer to the node context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeStartDynamicAllocation(UavcanNodeContext* ctx);

/**
 * @brief Process dynamic node ID allocation (should be called periodically)
 * @param ctx Pointer to the node context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanNodeProcessDynamicAllocation(UavcanNodeContext* ctx);

/**
 * @brief Check if dynamic node ID allocation is complete
 * @param ctx Pointer to the node context
 * @return true if allocation is complete, false otherwise
 */
bool uavcanNodeIsDynamicAllocationComplete(const UavcanNodeContext* ctx);

/**
 * @brief Get the dynamically allocated node ID
 * @param ctx Pointer to the node context
 * @return Allocated node ID (0 if not allocated)
 */
uint8_t uavcanNodeGetDynamicAllocatedId(const UavcanNodeContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_NODE_H
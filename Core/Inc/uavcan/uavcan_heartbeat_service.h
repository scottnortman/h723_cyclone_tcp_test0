#ifndef UAVCAN_HEARTBEAT_SERVICE_H
#define UAVCAN_HEARTBEAT_SERVICE_H

#include "uavcan_types.h"
#include "uavcan_message_handler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the UAVCAN heartbeat service
 * 
 * @param hb Pointer to heartbeat service structure
 * @param node_ctx Pointer to node context
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanHeartbeatInit(UavcanHeartbeatService* hb, UavcanNodeContext* node_ctx);

/**
 * @brief Start the heartbeat service
 * 
 * @param hb Pointer to heartbeat service structure
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanHeartbeatStart(UavcanHeartbeatService* hb);

/**
 * @brief Stop the heartbeat service
 * 
 * @param hb Pointer to heartbeat service structure
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanHeartbeatStop(UavcanHeartbeatService* hb);

/**
 * @brief Set the heartbeat interval
 * 
 * @param hb Pointer to heartbeat service structure
 * @param interval_ms Heartbeat interval in milliseconds
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanHeartbeatSetInterval(UavcanHeartbeatService* hb, uint32_t interval_ms);

/**
 * @brief Get the current heartbeat interval
 * 
 * @param hb Pointer to heartbeat service structure
 * @return Current heartbeat interval in milliseconds
 */
uint32_t uavcanHeartbeatGetInterval(const UavcanHeartbeatService* hb);

/**
 * @brief Check if heartbeat service is enabled
 * 
 * @param hb Pointer to heartbeat service structure
 * @return true if enabled, false otherwise
 */
bool uavcanHeartbeatIsEnabled(const UavcanHeartbeatService* hb);

/**
 * @brief Enable or disable the heartbeat service
 * 
 * @param hb Pointer to heartbeat service structure
 * @param enabled true to enable, false to disable
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanHeartbeatSetEnabled(UavcanHeartbeatService* hb, bool enabled);

/**
 * @brief Generate and send a heartbeat message immediately
 * 
 * @param hb Pointer to heartbeat service structure
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanHeartbeatSendNow(UavcanHeartbeatService* hb);

/**
 * @brief Validate heartbeat interval value
 * 
 * @param interval_ms Interval to validate in milliseconds
 * @return true if valid, false otherwise
 */
bool uavcanHeartbeatValidateInterval(uint32_t interval_ms);

/**
 * @brief Get heartbeat service status information
 * 
 * @param hb Pointer to heartbeat service structure
 * @param buffer Buffer to store status string
 * @param buffer_size Size of the buffer
 * @return Number of characters written to buffer
 */
size_t uavcanHeartbeatGetStatusString(const UavcanHeartbeatService* hb, char* buffer, size_t buffer_size);

/**
 * @brief Reset heartbeat service to default configuration
 * 
 * @param hb Pointer to heartbeat service structure
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanHeartbeatReset(UavcanHeartbeatService* hb);

// Heartbeat interval limits (as per UAVCAN specification)
#define UAVCAN_HEARTBEAT_INTERVAL_MIN_MS    100     // Minimum 100ms
#define UAVCAN_HEARTBEAT_INTERVAL_MAX_MS    60000   // Maximum 60 seconds
#define UAVCAN_HEARTBEAT_INTERVAL_DEFAULT_MS 1000   // Default 1 second

// Heartbeat task configuration
#define UAVCAN_HEARTBEAT_TASK_STACK_SIZE    512     // Stack size in words
#define UAVCAN_HEARTBEAT_TASK_PRIORITY      (tskIDLE_PRIORITY + 2)  // Low-Medium priority

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_HEARTBEAT_SERVICE_H
/**
 * @file uavcan_heartbeat.h
 * @brief UAVCAN Heartbeat Service interface
 * 
 * This file defines the interface for UAVCAN heartbeat functionality
 * including periodic transmission and configuration.
 */

#ifndef UAVCAN_HEARTBEAT_H
#define UAVCAN_HEARTBEAT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief UAVCAN Heartbeat structure
 */
struct UavcanHeartbeat {
    UavcanNode* node;
    systime_t interval_ms;
    OsTaskId task_id;
    OsTaskParameters task_params;
    bool running;
    bool initialized;
    uint32_t heartbeats_sent;
    systime_t last_heartbeat_time;
};

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Initialize UAVCAN heartbeat service
 * @param heartbeat Pointer to heartbeat structure
 * @param node Pointer to UAVCAN node structure
 * @retval UavcanError Error code
 */
UavcanError uavcanHeartbeatInit(UavcanHeartbeat* heartbeat, UavcanNode* node);

/**
 * @brief Deinitialize UAVCAN heartbeat service
 * @param heartbeat Pointer to heartbeat structure
 * @retval UavcanError Error code
 */
UavcanError uavcanHeartbeatDeinit(UavcanHeartbeat* heartbeat);

/**
 * @brief Start UAVCAN heartbeat service
 * @param heartbeat Pointer to heartbeat structure
 * @retval UavcanError Error code
 */
UavcanError uavcanHeartbeatStart(UavcanHeartbeat* heartbeat);

/**
 * @brief Stop UAVCAN heartbeat service
 * @param heartbeat Pointer to heartbeat structure
 * @retval UavcanError Error code
 */
UavcanError uavcanHeartbeatStop(UavcanHeartbeat* heartbeat);

/**
 * @brief Set heartbeat interval
 * @param heartbeat Pointer to heartbeat structure
 * @param interval_ms Interval in milliseconds
 * @retval UavcanError Error code
 */
UavcanError uavcanHeartbeatSetInterval(UavcanHeartbeat* heartbeat, systime_t interval_ms);

/**
 * @brief Get heartbeat interval
 * @param heartbeat Pointer to heartbeat structure
 * @retval systime_t Current interval in milliseconds
 */
systime_t uavcanHeartbeatGetInterval(const UavcanHeartbeat* heartbeat);

/**
 * @brief Send heartbeat message immediately
 * @param heartbeat Pointer to heartbeat structure
 * @retval UavcanError Error code
 */
UavcanError uavcanHeartbeatSendNow(UavcanHeartbeat* heartbeat);

/**
 * @brief Check if heartbeat service is running
 * @param heartbeat Pointer to heartbeat structure
 * @retval bool True if running, false otherwise
 */
bool uavcanHeartbeatIsRunning(const UavcanHeartbeat* heartbeat);

/**
 * @brief Get heartbeat statistics
 * @param heartbeat Pointer to heartbeat structure
 * @param heartbeats_sent Pointer to store number of heartbeats sent
 * @param last_time Pointer to store last heartbeat time
 * @retval UavcanError Error code
 */
UavcanError uavcanHeartbeatGetStats(const UavcanHeartbeat* heartbeat,
                                   uint32_t* heartbeats_sent,
                                   systime_t* last_time);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_HEARTBEAT_H */
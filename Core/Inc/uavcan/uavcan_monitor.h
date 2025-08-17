#ifndef UAVCAN_MONITOR_H
#define UAVCAN_MONITOR_H

#include "uavcan_types.h"
#include "uavcan_message_handler.h"
#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Monitor configuration
#define UAVCAN_MONITOR_MAX_MESSAGES     100
#define UAVCAN_MONITOR_BUFFER_SIZE      1024

// Monitor message entry
typedef struct {
    UavcanMessage message;
    uint64_t timestamp_usec;
    bool is_received;  // true for received, false for sent
} UavcanMonitorEntry;

// Monitor context
typedef struct {
    bool enabled;
    uint32_t message_count;
    uint32_t total_messages_monitored;
    UavcanMonitorEntry last_message;
    SemaphoreHandle_t mutex;
    // Circular buffer for message history (future enhancement)
    // UavcanMonitorEntry message_buffer[UAVCAN_MONITOR_MAX_MESSAGES];
    // uint32_t buffer_head;
    // uint32_t buffer_tail;
} UavcanMonitorContext;

/**
 * @brief Initialize the UAVCAN monitor
 * @param monitor Pointer to monitor context
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanMonitorInit(UavcanMonitorContext* monitor);

/**
 * @brief Enable UAVCAN message monitoring
 * @param monitor Pointer to monitor context
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanMonitorEnable(UavcanMonitorContext* monitor);

/**
 * @brief Disable UAVCAN message monitoring
 * @param monitor Pointer to monitor context
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanMonitorDisable(UavcanMonitorContext* monitor);

/**
 * @brief Check if monitoring is enabled
 * @param monitor Pointer to monitor context
 * @return bool true if enabled, false otherwise
 */
bool uavcanMonitorIsEnabled(const UavcanMonitorContext* monitor);

/**
 * @brief Log a received message to the monitor
 * @param monitor Pointer to monitor context
 * @param message Pointer to received message
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanMonitorLogReceived(UavcanMonitorContext* monitor, const UavcanMessage* message);

/**
 * @brief Log a sent message to the monitor
 * @param monitor Pointer to monitor context
 * @param message Pointer to sent message
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanMonitorLogSent(UavcanMonitorContext* monitor, const UavcanMessage* message);

/**
 * @brief Get monitor status as formatted string
 * @param monitor Pointer to monitor context
 * @param buffer Buffer to store status string
 * @param buffer_size Size of the buffer
 * @return size_t Number of characters written
 */
size_t uavcanMonitorGetStatusString(const UavcanMonitorContext* monitor, char* buffer, size_t buffer_size);

/**
 * @brief Reset monitor statistics
 * @param monitor Pointer to monitor context
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanMonitorReset(UavcanMonitorContext* monitor);

/**
 * @brief Get the last monitored message
 * @param monitor Pointer to monitor context
 * @param entry Pointer to store the last message entry
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanMonitorGetLastMessage(const UavcanMonitorContext* monitor, UavcanMonitorEntry* entry);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_MONITOR_H
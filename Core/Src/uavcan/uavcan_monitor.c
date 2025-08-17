#include "uavcan/uavcan_monitor.h"
#include "uavcan/uavcan_common.h"
#include <string.h>
#include <stdio.h>

// Global monitor instance (for simplicity)
static UavcanMonitorContext g_monitor_ctx = {0};

/**
 * @brief Get current timestamp in microseconds
 * @return uint64_t Current timestamp
 */
static uint64_t getCurrentTimestampUsec(void) {
    // Use FreeRTOS tick count converted to microseconds
    uint32_t ticks = xTaskGetTickCount();
    return (uint64_t)ticks * (1000000ULL / configTICK_RATE_HZ);
}

error_t uavcanMonitorInit(UavcanMonitorContext* monitor) {
    if (monitor == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize monitor context
    memset(monitor, 0, sizeof(UavcanMonitorContext));
    
    // Create mutex for thread safety
    monitor->mutex = xSemaphoreCreateMutex();
    if (monitor->mutex == NULL) {
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    monitor->enabled = false;
    monitor->message_count = 0;
    monitor->total_messages_monitored = 0;

    return UAVCAN_ERROR_NONE;
}

error_t uavcanMonitorEnable(UavcanMonitorContext* monitor) {
    if (monitor == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(monitor->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    monitor->enabled = true;

    xSemaphoreGive(monitor->mutex);
    return UAVCAN_ERROR_NONE;
}

error_t uavcanMonitorDisable(UavcanMonitorContext* monitor) {
    if (monitor == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(monitor->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    monitor->enabled = false;

    xSemaphoreGive(monitor->mutex);
    return UAVCAN_ERROR_NONE;
}

bool uavcanMonitorIsEnabled(const UavcanMonitorContext* monitor) {
    if (monitor == NULL) {
        return false;
    }
    return monitor->enabled;
}

error_t uavcanMonitorLogReceived(UavcanMonitorContext* monitor, const UavcanMessage* message) {
    if (monitor == NULL || message == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!monitor->enabled) {
        return UAVCAN_ERROR_NONE; // Not an error, just not monitoring
    }

    if (xSemaphoreTake(monitor->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Update statistics
    monitor->message_count++;
    monitor->total_messages_monitored++;

    // Store last message
    monitor->last_message.message = *message;
    monitor->last_message.timestamp_usec = getCurrentTimestampUsec();
    monitor->last_message.is_received = true;

    xSemaphoreGive(monitor->mutex);
    return UAVCAN_ERROR_NONE;
}

error_t uavcanMonitorLogSent(UavcanMonitorContext* monitor, const UavcanMessage* message) {
    if (monitor == NULL || message == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!monitor->enabled) {
        return UAVCAN_ERROR_NONE; // Not an error, just not monitoring
    }

    if (xSemaphoreTake(monitor->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Update statistics
    monitor->message_count++;
    monitor->total_messages_monitored++;

    // Store last message
    monitor->last_message.message = *message;
    monitor->last_message.timestamp_usec = getCurrentTimestampUsec();
    monitor->last_message.is_received = false;

    xSemaphoreGive(monitor->mutex);
    return UAVCAN_ERROR_NONE;
}

size_t uavcanMonitorGetStatusString(const UavcanMonitorContext* monitor, char* buffer, size_t buffer_size) {
    if (monitor == NULL || buffer == NULL || buffer_size == 0) {
        return 0;
    }

    size_t written = 0;
    
    if (xSemaphoreTake(monitor->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return snprintf(buffer, buffer_size, "Monitor status unavailable (mutex timeout)\r\n");
    }

    written = snprintf(buffer, buffer_size,
        "UAVCAN Message Monitoring Status:\r\n"
        "  Enabled: %s\r\n"
        "  Messages Monitored: %lu\r\n"
        "  Total Messages: %lu\r\n",
        monitor->enabled ? "Yes" : "No",
        (unsigned long)monitor->message_count,
        (unsigned long)monitor->total_messages_monitored);

    // Add last message information if available
    if (monitor->message_count > 0 && (written + 200) < buffer_size) {
        char temp_buffer[200];
        snprintf(temp_buffer, sizeof(temp_buffer),
            "  Last Message:\r\n"
            "    Direction: %s\r\n"
            "    Subject ID: %lu\r\n"
            "    Priority: %d (%s)\r\n"
            "    Source Node: %d\r\n"
            "    Payload Size: %zu bytes\r\n",
            monitor->last_message.is_received ? "Received" : "Sent",
            (unsigned long)monitor->last_message.message.subject_id,
            monitor->last_message.message.priority,
            uavcanPriorityToString(monitor->last_message.message.priority),
            monitor->last_message.message.source_node_id,
            monitor->last_message.message.payload_size);
        
        if (strlen(buffer) + strlen(temp_buffer) < buffer_size) {
            strcat(buffer, temp_buffer);
            written += strlen(temp_buffer);
        }
    } else if (monitor->message_count == 0) {
        if (written + 30 < buffer_size) {
            strcat(buffer, "  Last Message: None\r\n");
            written += 23;
        }
    }

    xSemaphoreGive(monitor->mutex);
    return written;
}

error_t uavcanMonitorReset(UavcanMonitorContext* monitor) {
    if (monitor == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(monitor->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    monitor->message_count = 0;
    monitor->total_messages_monitored = 0;
    memset(&monitor->last_message, 0, sizeof(UavcanMonitorEntry));

    xSemaphoreGive(monitor->mutex);
    return UAVCAN_ERROR_NONE;
}

error_t uavcanMonitorGetLastMessage(const UavcanMonitorContext* monitor, UavcanMonitorEntry* entry) {
    if (monitor == NULL || entry == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(monitor->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    if (monitor->message_count == 0) {
        xSemaphoreGive(monitor->mutex);
        return UAVCAN_ERROR_FAILURE; // No messages available
    }

    *entry = monitor->last_message;

    xSemaphoreGive(monitor->mutex);
    return UAVCAN_ERROR_NONE;
}

// Global monitor access functions
UavcanMonitorContext* uavcanGetGlobalMonitor(void) {
    return &g_monitor_ctx;
}

error_t uavcanInitGlobalMonitor(void) {
    return uavcanMonitorInit(&g_monitor_ctx);
}
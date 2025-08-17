#include "uavcan/uavcan_heartbeat_service.h"
#include "uavcan/uavcan_priority_queue.h"
#include <string.h>
#include <stdio.h>

// External reference to the global priority queue
// This would typically be defined in the main UAVCAN system
extern UavcanPriorityQueue* g_uavcan_priority_queue;

// Forward declaration of heartbeat task function
static void uavcanHeartbeatTask(void* pvParameters);

// Forward declaration of timer callback function
static void uavcanHeartbeatTimerCallback(TimerHandle_t xTimer);

error_t uavcanHeartbeatInit(UavcanHeartbeatService* hb, UavcanNodeContext* node_ctx)
{
    if (hb == NULL || node_ctx == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize heartbeat service structure
    memset(hb, 0, sizeof(UavcanHeartbeatService));
    
    hb->interval_ms = UAVCAN_HEARTBEAT_INTERVAL_DEFAULT_MS;
    hb->enabled = false;
    hb->task_handle = NULL;
    hb->node_ctx = node_ctx;

    return NO_ERROR;
}

error_t uavcanHeartbeatStart(UavcanHeartbeatService* hb)
{
    if (hb == NULL || hb->node_ctx == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (hb->enabled) {
        // Already started
        return NO_ERROR;
    }

    // Create heartbeat task
    BaseType_t result = xTaskCreate(
        uavcanHeartbeatTask,                    // Task function
        "UAVCAN_HB",                           // Task name
        UAVCAN_HEARTBEAT_TASK_STACK_SIZE,      // Stack size
        (void*)hb,                             // Task parameter
        UAVCAN_HEARTBEAT_TASK_PRIORITY,        // Task priority
        &hb->task_handle                       // Task handle
    );

    if (result != pdPASS) {
        return ERROR_FAILURE;
    }

    hb->enabled = true;
    return NO_ERROR;
}

error_t uavcanHeartbeatStop(UavcanHeartbeatService* hb)
{
    if (hb == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!hb->enabled) {
        // Already stopped
        return NO_ERROR;
    }

    // Delete the task if it exists
    if (hb->task_handle != NULL) {
        vTaskDelete(hb->task_handle);
        hb->task_handle = NULL;
    }

    hb->enabled = false;
    return NO_ERROR;
}

error_t uavcanHeartbeatSetInterval(UavcanHeartbeatService* hb, uint32_t interval_ms)
{
    if (hb == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!uavcanHeartbeatValidateInterval(interval_ms)) {
        return ERROR_INVALID_PARAMETER;
    }

    hb->interval_ms = interval_ms;
    
    // If the service is running, restart it with the new interval
    if (hb->enabled) {
        error_t result = uavcanHeartbeatStop(hb);
        if (result != NO_ERROR) {
            return result;
        }
        
        result = uavcanHeartbeatStart(hb);
        if (result != NO_ERROR) {
            return result;
        }
    }

    return NO_ERROR;
}

uint32_t uavcanHeartbeatGetInterval(const UavcanHeartbeatService* hb)
{
    if (hb == NULL) {
        return 0;
    }

    return hb->interval_ms;
}

bool uavcanHeartbeatIsEnabled(const UavcanHeartbeatService* hb)
{
    if (hb == NULL) {
        return false;
    }

    return hb->enabled;
}

error_t uavcanHeartbeatSetEnabled(UavcanHeartbeatService* hb, bool enabled)
{
    if (hb == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (enabled && !hb->enabled) {
        return uavcanHeartbeatStart(hb);
    } else if (!enabled && hb->enabled) {
        return uavcanHeartbeatStop(hb);
    }

    return NO_ERROR;
}

error_t uavcanHeartbeatSendNow(UavcanHeartbeatService* hb)
{
    if (hb == NULL || hb->node_ctx == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Create heartbeat message
    UavcanMessage heartbeat_msg;
    error_t result = uavcanMessageCreateHeartbeat(
        &heartbeat_msg,
        uavcanNodeGetHealth(hb->node_ctx),
        uavcanNodeGetMode(hb->node_ctx),
        uavcanNodeGetUptime(hb->node_ctx)
    );

    if (result != NO_ERROR) {
        return result;
    }

    // Set source node ID
    heartbeat_msg.source_node_id = uavcanNodeGetId(hb->node_ctx);

    // Send message through priority queue system
    if (g_uavcan_priority_queue != NULL) {
        UavcanError queue_result = uavcanPriorityQueuePush(g_uavcan_priority_queue, &heartbeat_msg);
        if (queue_result != UAVCAN_ERROR_NONE) {
            // Clean up message on queue failure
            uavcanMessageDestroy(&heartbeat_msg);
            return (queue_result == UAVCAN_ERROR_QUEUE_FULL) ? ERROR_FAILURE : ERROR_FAILURE;
        }
    } else {
        // Priority queue not available - this is an error condition
        uavcanMessageDestroy(&heartbeat_msg);
        return ERROR_FAILURE;
    }
    
    // Note: Message cleanup will be handled by the priority queue system
    // after transmission, so we don't call uavcanMessageDestroy here

    return NO_ERROR;
}

bool uavcanHeartbeatValidateInterval(uint32_t interval_ms)
{
    return (interval_ms >= UAVCAN_HEARTBEAT_INTERVAL_MIN_MS && 
            interval_ms <= UAVCAN_HEARTBEAT_INTERVAL_MAX_MS);
}

size_t uavcanHeartbeatGetStatusString(const UavcanHeartbeatService* hb, char* buffer, size_t buffer_size)
{
    if (hb == NULL || buffer == NULL || buffer_size == 0) {
        return 0;
    }

    return (size_t)snprintf(buffer, buffer_size,
        "Heartbeat Service Status:\n"
        "  Enabled: %s\n"
        "  Interval: %lu ms\n"
        "  Task Handle: %p\n",
        hb->enabled ? "Yes" : "No",
        (unsigned long)hb->interval_ms,
        hb->task_handle
    );
}

error_t uavcanHeartbeatReset(UavcanHeartbeatService* hb)
{
    if (hb == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Stop the service if running
    if (hb->enabled) {
        error_t result = uavcanHeartbeatStop(hb);
        if (result != NO_ERROR) {
            return result;
        }
    }

    // Reset to default values
    hb->interval_ms = UAVCAN_HEARTBEAT_INTERVAL_DEFAULT_MS;
    hb->enabled = false;
    hb->task_handle = NULL;
    // Note: node_ctx is not reset as it's a reference

    return NO_ERROR;
}

/**
 * @brief Heartbeat task function
 * 
 * This task runs periodically and sends heartbeat messages at the configured interval.
 * 
 * @param pvParameters Pointer to UavcanHeartbeatService structure
 */
static void uavcanHeartbeatTask(void* pvParameters)
{
    UavcanHeartbeatService* hb = (UavcanHeartbeatService*)pvParameters;
    
    if (hb == NULL) {
        // Invalid parameter, delete task
        vTaskDelete(NULL);
        return;
    }

    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xFrequency = pdMS_TO_TICKS(hb->interval_ms);

    while (hb->enabled) {
        // Wait for the next cycle
        vTaskDelayUntil(&xLastWakeTime, xFrequency);

        // Send heartbeat message
        error_t result = uavcanHeartbeatSendNow(hb);
        
        // Log error if heartbeat sending failed (in a real implementation)
        if (result != NO_ERROR) {
            // TODO: Add proper error logging
            // For now, continue trying
        }

        // Update node uptime
        uavcanNodeUpdateUptime(hb->node_ctx);
    }

    // Task is ending, clean up
    hb->task_handle = NULL;
    vTaskDelete(NULL);
}
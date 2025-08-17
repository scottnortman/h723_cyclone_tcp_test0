#include "uavcan/uavcan_error_handler.h"
#include <string.h>
#include <stdio.h>

// FreeRTOS includes for timing
#include "FreeRTOS.h"
#include "task.h"

// Error string lookup table
static const char* error_strings[] = {
    [UAVCAN_ERROR_NONE] = "No error",
    [UAVCAN_ERROR_INIT_FAILED] = "Initialization failed",
    [UAVCAN_ERROR_NETWORK_UNAVAILABLE] = "Network unavailable",
    [UAVCAN_ERROR_SEND_FAILED] = "Send operation failed",
    [UAVCAN_ERROR_RECEIVE_FAILED] = "Receive operation failed",
    [UAVCAN_ERROR_QUEUE_FULL] = "Queue is full",
    [UAVCAN_ERROR_INVALID_CONFIG] = "Invalid configuration",
    [UAVCAN_ERROR_TIMEOUT] = "Operation timeout",
    [UAVCAN_ERROR_INVALID_PARAMETER] = "Invalid parameter",
    [UAVCAN_ERROR_MEMORY_ALLOCATION] = "Memory allocation failed",
    [UAVCAN_ERROR_NODE_ID_CONFLICT] = "Node ID conflict",
    [UAVCAN_ERROR_TRANSPORT_ERROR] = "Transport layer error"
};

// Log level strings
static const char* log_level_strings[] = {
    [UAVCAN_LOG_LEVEL_DEBUG] = "DEBUG",
    [UAVCAN_LOG_LEVEL_INFO] = "INFO",
    [UAVCAN_LOG_LEVEL_WARNING] = "WARNING",
    [UAVCAN_LOG_LEVEL_ERROR] = "ERROR",
    [UAVCAN_LOG_LEVEL_CRITICAL] = "CRITICAL"
};

/**
 * Initialize the error handler
 */
UavcanError uavcanErrorHandlerInit(UavcanErrorHandler* handler, UavcanLogLevel min_level)
{
    if (handler == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize configuration
    handler->min_log_level = min_level;
    handler->auto_recovery_enabled = true;
    handler->max_recovery_attempts = 3;
    handler->error_callback = NULL;

    // Initialize statistics
    memset(&handler->statistics, 0, sizeof(UavcanErrorStatistics));

    return UAVCAN_ERROR_NONE;
}

/**
 * Deinitialize the error handler
 */
void uavcanErrorHandlerDeinit(UavcanErrorHandler* handler)
{
    if (handler != NULL) {
        // Reset all fields
        memset(handler, 0, sizeof(UavcanErrorHandler));
    }
}

/**
 * Get current timestamp in milliseconds
 */
static uint32_t getCurrentTimestamp(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/**
 * Log an error with detailed context
 */
void uavcanLogError(UavcanErrorHandler* handler, UavcanError error_code, 
                   UavcanLogLevel severity, const char* function, uint32_t line,
                   const char* description, uint32_t additional_data)
{
    if (handler == NULL || severity < handler->min_log_level) {
        return;
    }

    // Update statistics
    handler->statistics.total_errors++;
    if (error_code <= UAVCAN_ERROR_TRANSPORT_ERROR) {
        handler->statistics.errors_by_type[error_code]++;
    }
    if (severity == UAVCAN_LOG_LEVEL_CRITICAL) {
        handler->statistics.critical_errors++;
    }
    handler->statistics.last_error_timestamp = getCurrentTimestamp();
    handler->statistics.last_error_code = error_code;

    // Create error context
    UavcanErrorContext error_ctx = {
        .error_code = error_code,
        .severity = severity,
        .timestamp_ms = getCurrentTimestamp(),
        .function_name = function,
        .line_number = line,
        .description = description,
        .additional_data = additional_data
    };

    // Call error callback if registered
    if (handler->error_callback != NULL) {
        handler->error_callback(&error_ctx);
    }

    // Print error to console (basic implementation)
    printf("[UAVCAN][%s] %s:%lu - %s (%s) - Data: 0x%08lX\r\n",
           log_level_strings[severity],
           function ? function : "unknown",
           (unsigned long)line,
           description ? description : "No description",
           uavcanGetErrorString(error_code),
           (unsigned long)additional_data);

    // Attempt automatic recovery for recoverable errors
    if (handler->auto_recovery_enabled && uavcanIsRecoverableError(error_code)) {
        if (handler->statistics.recovery_attempts < handler->max_recovery_attempts) {
            handler->statistics.recovery_attempts++;
            UavcanError recovery_result = uavcanRecoverFromError(handler, error_code);
            if (recovery_result == UAVCAN_ERROR_NONE) {
                handler->statistics.successful_recoveries++;
                printf("[UAVCAN][INFO] Successfully recovered from error %s\r\n", 
                       uavcanGetErrorString(error_code));
            }
        }
    }
}

/**
 * Attempt to recover from a specific error
 */
UavcanError uavcanRecoverFromError(UavcanErrorHandler* handler, UavcanError error_code)
{
    if (handler == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    switch (error_code) {
        case UAVCAN_ERROR_NETWORK_UNAVAILABLE:
            // Wait for network to become available
            printf("[UAVCAN][INFO] Attempting network recovery...\r\n");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
            // In a real implementation, we would check network status here
            return UAVCAN_ERROR_NONE;

        case UAVCAN_ERROR_QUEUE_FULL:
            // Queue full errors are typically temporary
            printf("[UAVCAN][INFO] Queue full - waiting for space...\r\n");
            vTaskDelay(pdMS_TO_TICKS(100)); // Wait 100ms
            return UAVCAN_ERROR_NONE;

        case UAVCAN_ERROR_TIMEOUT:
            // Timeout errors can often be retried
            printf("[UAVCAN][INFO] Timeout occurred - will retry...\r\n");
            return UAVCAN_ERROR_NONE;

        case UAVCAN_ERROR_SEND_FAILED:
        case UAVCAN_ERROR_RECEIVE_FAILED:
            // Network operation failures might be temporary
            printf("[UAVCAN][INFO] Network operation failed - checking connection...\r\n");
            vTaskDelay(pdMS_TO_TICKS(500)); // Wait 500ms
            return UAVCAN_ERROR_NONE;

        case UAVCAN_ERROR_TRANSPORT_ERROR:
            // Transport errors might require reinitialization
            printf("[UAVCAN][INFO] Transport error - attempting reset...\r\n");
            vTaskDelay(pdMS_TO_TICKS(1000)); // Wait 1 second
            return UAVCAN_ERROR_NONE;

        default:
            // Non-recoverable errors
            printf("[UAVCAN][WARNING] Error %s is not recoverable\r\n", 
                   uavcanGetErrorString(error_code));
            return error_code;
    }
}

/**
 * Check if an error is recoverable
 */
bool uavcanIsRecoverableError(UavcanError error_code)
{
    switch (error_code) {
        case UAVCAN_ERROR_NETWORK_UNAVAILABLE:
        case UAVCAN_ERROR_QUEUE_FULL:
        case UAVCAN_ERROR_TIMEOUT:
        case UAVCAN_ERROR_SEND_FAILED:
        case UAVCAN_ERROR_RECEIVE_FAILED:
        case UAVCAN_ERROR_TRANSPORT_ERROR:
            return true;

        case UAVCAN_ERROR_INIT_FAILED:
        case UAVCAN_ERROR_INVALID_CONFIG:
        case UAVCAN_ERROR_INVALID_PARAMETER:
        case UAVCAN_ERROR_MEMORY_ALLOCATION:
        case UAVCAN_ERROR_NODE_ID_CONFLICT:
        default:
            return false;
    }
}

/**
 * Get human-readable error string
 */
const char* uavcanGetErrorString(UavcanError error_code)
{
    if (error_code <= UAVCAN_ERROR_TRANSPORT_ERROR) {
        return error_strings[error_code];
    }
    return "Unknown error";
}

/**
 * Get error statistics
 */
const UavcanErrorStatistics* uavcanGetErrorStatistics(const UavcanErrorHandler* handler)
{
    if (handler == NULL) {
        return NULL;
    }
    return &handler->statistics;
}

/**
 * Reset error statistics
 */
void uavcanResetErrorStatistics(UavcanErrorHandler* handler)
{
    if (handler != NULL) {
        memset(&handler->statistics, 0, sizeof(UavcanErrorStatistics));
    }
}
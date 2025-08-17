#include "uavcan/uavcan_system_stability.h"
#include <string.h>
#include <stdio.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"

// State transition strings for logging
static const char* stability_state_strings[] = {
    [UAVCAN_STABILITY_NORMAL] = "NORMAL",
    [UAVCAN_STABILITY_DEGRADED] = "DEGRADED", 
    [UAVCAN_STABILITY_ISOLATED] = "ISOLATED",
    [UAVCAN_STABILITY_FAILED] = "FAILED"
};

/**
 * Get current timestamp in milliseconds
 */
static uint32_t getCurrentTimestamp(void)
{
    return (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
}

/**
 * Initialize the stability manager
 */
UavcanError uavcanStabilityInit(UavcanStabilityManager* manager, 
                               UavcanErrorHandler* error_handler)
{
    if (manager == NULL || error_handler == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize manager state
    manager->current_state = UAVCAN_STABILITY_NORMAL;
    manager->error_handler = error_handler;
    manager->isolation_enabled = true;
    manager->failure_threshold = 5;  // Isolate after 5 failures
    manager->recovery_timeout_ms = 30000;  // 30 seconds between recovery attempts
    manager->last_recovery_attempt = 0;
    
    // Initialize task monitoring
    memset(manager->task_health, 0, sizeof(manager->task_health));
    manager->monitored_task_count = 0;
    
    // Initialize statistics
    manager->isolation_events = 0;
    manager->recovery_attempts = 0;
    manager->successful_recoveries = 0;
    manager->total_uptime_ms = 0;
    manager->degraded_time_ms = 0;

    UAVCAN_LOG_INFO(error_handler, UAVCAN_ERROR_NONE, 
                    "Stability manager initialized", 0);

    return UAVCAN_ERROR_NONE;
}

/**
 * Deinitialize the stability manager
 */
void uavcanStabilityDeinit(UavcanStabilityManager* manager)
{
    if (manager != NULL) {
        if (manager->error_handler != NULL) {
            UAVCAN_LOG_INFO(manager->error_handler, UAVCAN_ERROR_NONE,
                            "Stability manager deinitialized", 0);
        }
        memset(manager, 0, sizeof(UavcanStabilityManager));
    }
}

/**
 * Register a task for health monitoring
 */
UavcanError uavcanStabilityRegisterTask(UavcanStabilityManager* manager,
                                       void* task_handle, const char* task_name,
                                       uint32_t heartbeat_interval_ms)
{
    if (manager == NULL || task_handle == NULL || task_name == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (manager->monitored_task_count >= 4) {
        UAVCAN_LOG_ERROR(manager->error_handler, UAVCAN_ERROR_INVALID_CONFIG,
                         "Maximum monitored tasks exceeded", manager->monitored_task_count);
        return UAVCAN_ERROR_INVALID_CONFIG;
    }

    UavcanTaskHealth* task_health = &manager->task_health[manager->monitored_task_count];
    
    task_health->task_handle = task_handle;
    task_health->task_name = task_name;
    task_health->last_heartbeat_time = getCurrentTimestamp();
    task_health->heartbeat_interval_ms = heartbeat_interval_ms;
    task_health->missed_heartbeats = 0;
    task_health->is_healthy = true;
    
    // Initialize task watchdog
    uavcanWatchdogInit(&task_health->watchdog, heartbeat_interval_ms * 3); // 3x interval timeout
    
    manager->monitored_task_count++;

    UAVCAN_LOG_INFO(manager->error_handler, UAVCAN_ERROR_NONE,
                    "Task registered for monitoring", manager->monitored_task_count);

    return UAVCAN_ERROR_NONE;
}

/**
 * Record a task heartbeat
 */
void uavcanStabilityTaskHeartbeat(UavcanStabilityManager* manager, void* task_handle)
{
    if (manager == NULL || task_handle == NULL) {
        return;
    }

    // Find the task in our monitoring list
    for (uint8_t i = 0; i < manager->monitored_task_count; i++) {
        if (manager->task_health[i].task_handle == task_handle) {
            manager->task_health[i].last_heartbeat_time = getCurrentTimestamp();
            manager->task_health[i].missed_heartbeats = 0;
            manager->task_health[i].is_healthy = true;
            uavcanWatchdogKick(&manager->task_health[i].watchdog);
            return;
        }
    }
}

/**
 * Check health of all monitored tasks
 */
void uavcanStabilityCheckTaskHealth(UavcanStabilityManager* manager)
{
    if (manager == NULL) {
        return;
    }

    uint32_t current_time = getCurrentTimestamp();
    bool any_task_unhealthy = false;

    for (uint8_t i = 0; i < manager->monitored_task_count; i++) {
        UavcanTaskHealth* task = &manager->task_health[i];
        
        // Check if task has missed its heartbeat
        uint32_t time_since_heartbeat = current_time - task->last_heartbeat_time;
        if (time_since_heartbeat > task->heartbeat_interval_ms * 2) { // 2x interval tolerance
            task->missed_heartbeats++;
            
            if (task->is_healthy) {
                task->is_healthy = false;
                any_task_unhealthy = true;
                
                UAVCAN_LOG_WARNING(manager->error_handler, UAVCAN_ERROR_TIMEOUT,
                                   "Task heartbeat missed", (uint32_t)task->task_name);
            }
        }
        
        // Check watchdog status
        if (uavcanWatchdogIsExpired(&task->watchdog)) {
            if (task->is_healthy) {
                task->is_healthy = false;
                any_task_unhealthy = true;
                
                UAVCAN_LOG_ERROR(manager->error_handler, UAVCAN_ERROR_TIMEOUT,
                                 "Task watchdog expired", (uint32_t)task->task_name);
            }
        }
    }

    // If any task is unhealthy and we're in normal state, enter degraded mode
    if (any_task_unhealthy && manager->current_state == UAVCAN_STABILITY_NORMAL) {
        uavcanStabilityEnterDegradedMode(manager);
    }
}

/**
 * Initialize a watchdog
 */
UavcanError uavcanWatchdogInit(UavcanWatchdog* watchdog, uint32_t timeout_ms)
{
    if (watchdog == NULL || timeout_ms == 0) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    watchdog->timeout_ms = timeout_ms;
    watchdog->last_kick_time = getCurrentTimestamp();
    watchdog->enabled = true;
    watchdog->timeout_count = 0;

    return UAVCAN_ERROR_NONE;
}

/**
 * Kick (reset) a watchdog
 */
void uavcanWatchdogKick(UavcanWatchdog* watchdog)
{
    if (watchdog != NULL && watchdog->enabled) {
        watchdog->last_kick_time = getCurrentTimestamp();
    }
}

/**
 * Check if a watchdog has expired
 */
bool uavcanWatchdogIsExpired(const UavcanWatchdog* watchdog)
{
    if (watchdog == NULL || !watchdog->enabled) {
        return false;
    }

    uint32_t current_time = getCurrentTimestamp();
    uint32_t elapsed = current_time - watchdog->last_kick_time;
    
    return elapsed > watchdog->timeout_ms;
}

/**
 * Reset a watchdog after timeout
 */
void uavcanWatchdogReset(UavcanWatchdog* watchdog)
{
    if (watchdog != NULL) {
        watchdog->timeout_count++;
        watchdog->last_kick_time = getCurrentTimestamp();
    }
}

/**
 * Get current stability state
 */
UavcanStabilityState uavcanStabilityGetState(const UavcanStabilityManager* manager)
{
    if (manager == NULL) {
        return UAVCAN_STABILITY_FAILED;
    }
    return manager->current_state;
}

/**
 * Set stability state with logging
 */
UavcanError uavcanStabilitySetState(UavcanStabilityManager* manager, 
                                   UavcanStabilityState new_state)
{
    if (manager == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (manager->current_state != new_state) {
        UavcanStabilityState old_state = manager->current_state;
        manager->current_state = new_state;

        UAVCAN_LOG_INFO(manager->error_handler, UAVCAN_ERROR_NONE,
                        "Stability state changed", 
                        (old_state << 16) | new_state);

        printf("[UAVCAN][STABILITY] State changed: %s -> %s\r\n",
               stability_state_strings[old_state],
               stability_state_strings[new_state]);
    }

    return UAVCAN_ERROR_NONE;
}

/**
 * Check if system is operational
 */
bool uavcanStabilityIsOperational(const UavcanStabilityManager* manager)
{
    if (manager == NULL) {
        return false;
    }
    
    return (manager->current_state == UAVCAN_STABILITY_NORMAL ||
            manager->current_state == UAVCAN_STABILITY_DEGRADED);
}

/**
 * Handle an error and determine if isolation is needed
 */
UavcanError uavcanStabilityHandleError(UavcanStabilityManager* manager, 
                                      UavcanError error_code)
{
    if (manager == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Check if this is a critical error that requires isolation
    bool should_isolate = false;
    
    switch (error_code) {
        case UAVCAN_ERROR_INIT_FAILED:
        case UAVCAN_ERROR_MEMORY_ALLOCATION:
        case UAVCAN_ERROR_NODE_ID_CONFLICT:
            should_isolate = true;
            break;
            
        default:
            // Check failure threshold for other errors
            const UavcanErrorStatistics* stats = uavcanGetErrorStatistics(manager->error_handler);
            if (stats != NULL && stats->total_errors >= manager->failure_threshold) {
                should_isolate = true;
            }
            break;
    }

    if (should_isolate && manager->isolation_enabled) {
        return uavcanStabilityIsolateSubsystem(manager);
    } else if (manager->current_state == UAVCAN_STABILITY_NORMAL) {
        return uavcanStabilityEnterDegradedMode(manager);
    }

    return UAVCAN_ERROR_NONE;
}

/**
 * Isolate the UAVCAN subsystem
 */
UavcanError uavcanStabilityIsolateSubsystem(UavcanStabilityManager* manager)
{
    if (manager == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    manager->isolation_events++;
    uavcanStabilitySetState(manager, UAVCAN_STABILITY_ISOLATED);

    UAVCAN_LOG_CRITICAL(manager->error_handler, UAVCAN_ERROR_NONE,
                        "UAVCAN subsystem isolated", manager->isolation_events);

    printf("[UAVCAN][STABILITY] SUBSYSTEM ISOLATED - Main system protected\r\n");

    return UAVCAN_ERROR_NONE;
}

/**
 * Attempt to recover from isolation or failure
 */
UavcanError uavcanStabilityAttemptRecovery(UavcanStabilityManager* manager)
{
    if (manager == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    uint32_t current_time = getCurrentTimestamp();
    
    // Check if enough time has passed since last recovery attempt
    if (current_time - manager->last_recovery_attempt < manager->recovery_timeout_ms) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    manager->recovery_attempts++;
    manager->last_recovery_attempt = current_time;

    UAVCAN_LOG_INFO(manager->error_handler, UAVCAN_ERROR_NONE,
                    "Attempting system recovery", manager->recovery_attempts);

    // Reset error statistics to give the system a fresh start
    uavcanResetErrorStatistics(manager->error_handler);

    // Reset all task watchdogs
    for (uint8_t i = 0; i < manager->monitored_task_count; i++) {
        uavcanWatchdogReset(&manager->task_health[i].watchdog);
        manager->task_health[i].is_healthy = true;
        manager->task_health[i].missed_heartbeats = 0;
    }

    // Attempt to return to normal operation
    UavcanError result = uavcanStabilitySetState(manager, UAVCAN_STABILITY_NORMAL);
    if (result == UAVCAN_ERROR_NONE) {
        manager->successful_recoveries++;
        printf("[UAVCAN][STABILITY] Recovery successful\r\n");
    }

    return result;
}

/**
 * Enter degraded mode
 */
UavcanError uavcanStabilityEnterDegradedMode(UavcanStabilityManager* manager)
{
    if (manager == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    uavcanStabilitySetState(manager, UAVCAN_STABILITY_DEGRADED);

    UAVCAN_LOG_WARNING(manager->error_handler, UAVCAN_ERROR_NONE,
                       "Entering degraded mode", 0);

    printf("[UAVCAN][STABILITY] Entering degraded mode - reduced functionality\r\n");

    return UAVCAN_ERROR_NONE;
}

/**
 * Exit degraded mode
 */
UavcanError uavcanStabilityExitDegradedMode(UavcanStabilityManager* manager)
{
    if (manager == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (manager->current_state == UAVCAN_STABILITY_DEGRADED) {
        uavcanStabilitySetState(manager, UAVCAN_STABILITY_NORMAL);
        
        UAVCAN_LOG_INFO(manager->error_handler, UAVCAN_ERROR_NONE,
                        "Exiting degraded mode", 0);
        
        printf("[UAVCAN][STABILITY] Returning to normal operation\r\n");
    }

    return UAVCAN_ERROR_NONE;
}

/**
 * Get stability statistics
 */
const UavcanStabilityStatistics* uavcanStabilityGetStatistics(const UavcanStabilityManager* manager)
{
    static UavcanStabilityStatistics stats;
    
    if (manager == NULL) {
        return NULL;
    }

    stats.current_state = manager->current_state;
    stats.isolation_events = manager->isolation_events;
    stats.recovery_attempts = manager->recovery_attempts;
    stats.successful_recoveries = manager->successful_recoveries;
    stats.total_uptime_ms = manager->total_uptime_ms;
    stats.degraded_time_ms = manager->degraded_time_ms;
    
    // Count healthy tasks
    stats.healthy_tasks = 0;
    stats.total_tasks = manager->monitored_task_count;
    stats.total_watchdog_timeouts = 0;
    
    for (uint8_t i = 0; i < manager->monitored_task_count; i++) {
        if (manager->task_health[i].is_healthy) {
            stats.healthy_tasks++;
        }
        stats.total_watchdog_timeouts += manager->task_health[i].watchdog.timeout_count;
    }

    return &stats;
}

/**
 * Reset stability statistics
 */
void uavcanStabilityResetStatistics(UavcanStabilityManager* manager)
{
    if (manager != NULL) {
        manager->isolation_events = 0;
        manager->recovery_attempts = 0;
        manager->successful_recoveries = 0;
        manager->total_uptime_ms = 0;
        manager->degraded_time_ms = 0;
        
        // Reset task statistics
        for (uint8_t i = 0; i < manager->monitored_task_count; i++) {
            manager->task_health[i].missed_heartbeats = 0;
            manager->task_health[i].watchdog.timeout_count = 0;
        }
    }
}

/**
 * Periodic update function - should be called regularly
 */
void uavcanStabilityUpdate(UavcanStabilityManager* manager)
{
    if (manager == NULL) {
        return;
    }

    static uint32_t last_update_time = 0;
    uint32_t current_time = getCurrentTimestamp();
    
    // Update uptime statistics
    if (last_update_time != 0) {
        uint32_t elapsed = current_time - last_update_time;
        manager->total_uptime_ms += elapsed;
        
        if (manager->current_state == UAVCAN_STABILITY_DEGRADED) {
            manager->degraded_time_ms += elapsed;
        }
    }
    last_update_time = current_time;

    // Check task health
    uavcanStabilityCheckTaskHealth(manager);

    // Attempt automatic recovery if in isolated state and enough time has passed
    if (manager->current_state == UAVCAN_STABILITY_ISOLATED) {
        if (current_time - manager->last_recovery_attempt >= manager->recovery_timeout_ms) {
            uavcanStabilityAttemptRecovery(manager);
        }
    }
}
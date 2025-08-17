#ifndef UAVCAN_SYSTEM_STABILITY_H
#define UAVCAN_SYSTEM_STABILITY_H

#include "uavcan_types.h"
#include "uavcan_error_handler.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// System stability states
typedef enum {
    UAVCAN_STABILITY_NORMAL = 0,        // Normal operation
    UAVCAN_STABILITY_DEGRADED = 1,      // Degraded operation due to errors
    UAVCAN_STABILITY_ISOLATED = 2,      // UAVCAN subsystem isolated
    UAVCAN_STABILITY_FAILED = 3         // Complete subsystem failure
} UavcanStabilityState;

// Watchdog configuration
typedef struct {
    uint32_t timeout_ms;                // Watchdog timeout in milliseconds
    uint32_t last_kick_time;            // Last watchdog kick timestamp
    bool enabled;                       // Watchdog enabled flag
    uint32_t timeout_count;             // Number of timeouts occurred
} UavcanWatchdog;

// Task health monitoring
typedef struct {
    void* task_handle;                  // FreeRTOS task handle
    const char* task_name;              // Task name for identification
    uint32_t last_heartbeat_time;       // Last heartbeat timestamp
    uint32_t heartbeat_interval_ms;     // Expected heartbeat interval
    uint32_t missed_heartbeats;         // Count of missed heartbeats
    bool is_healthy;                    // Current health status
    UavcanWatchdog watchdog;            // Task-specific watchdog
} UavcanTaskHealth;

// System stability manager
typedef struct {
    UavcanStabilityState current_state; // Current system state
    UavcanErrorHandler* error_handler;  // Reference to error handler
    bool isolation_enabled;             // Error isolation enabled
    uint32_t failure_threshold;         // Failure count threshold for isolation
    uint32_t recovery_timeout_ms;       // Time to wait before recovery attempt
    uint32_t last_recovery_attempt;     // Timestamp of last recovery attempt
    
    // Task monitoring
    UavcanTaskHealth task_health[4];    // Monitor up to 4 UAVCAN tasks
    uint8_t monitored_task_count;       // Number of monitored tasks
    
    // System statistics
    uint32_t isolation_events;          // Number of isolation events
    uint32_t recovery_attempts;         // Number of recovery attempts
    uint32_t successful_recoveries;     // Number of successful recoveries
    uint64_t total_uptime_ms;           // Total system uptime
    uint64_t degraded_time_ms;          // Time spent in degraded state
} UavcanStabilityManager;

// Stability manager functions
UavcanError uavcanStabilityInit(UavcanStabilityManager* manager, 
                               UavcanErrorHandler* error_handler);
void uavcanStabilityDeinit(UavcanStabilityManager* manager);

// Task monitoring functions
UavcanError uavcanStabilityRegisterTask(UavcanStabilityManager* manager,
                                       void* task_handle, const char* task_name,
                                       uint32_t heartbeat_interval_ms);
void uavcanStabilityTaskHeartbeat(UavcanStabilityManager* manager, void* task_handle);
void uavcanStabilityCheckTaskHealth(UavcanStabilityManager* manager);

// Watchdog functions
UavcanError uavcanWatchdogInit(UavcanWatchdog* watchdog, uint32_t timeout_ms);
void uavcanWatchdogKick(UavcanWatchdog* watchdog);
bool uavcanWatchdogIsExpired(const UavcanWatchdog* watchdog);
void uavcanWatchdogReset(UavcanWatchdog* watchdog);

// System state management
UavcanStabilityState uavcanStabilityGetState(const UavcanStabilityManager* manager);
UavcanError uavcanStabilitySetState(UavcanStabilityManager* manager, 
                                   UavcanStabilityState new_state);
bool uavcanStabilityIsOperational(const UavcanStabilityManager* manager);

// Error isolation and recovery
UavcanError uavcanStabilityHandleError(UavcanStabilityManager* manager, 
                                      UavcanError error_code);
UavcanError uavcanStabilityIsolateSubsystem(UavcanStabilityManager* manager);
UavcanError uavcanStabilityAttemptRecovery(UavcanStabilityManager* manager);

// Graceful degradation
UavcanError uavcanStabilityEnterDegradedMode(UavcanStabilityManager* manager);
UavcanError uavcanStabilityExitDegradedMode(UavcanStabilityManager* manager);

// Statistics and monitoring
typedef struct {
    UavcanStabilityState current_state;
    uint32_t isolation_events;
    uint32_t recovery_attempts;
    uint32_t successful_recoveries;
    uint64_t total_uptime_ms;
    uint64_t degraded_time_ms;
    uint8_t healthy_tasks;
    uint8_t total_tasks;
    uint32_t total_watchdog_timeouts;
} UavcanStabilityStatistics;

const UavcanStabilityStatistics* uavcanStabilityGetStatistics(const UavcanStabilityManager* manager);
void uavcanStabilityResetStatistics(UavcanStabilityManager* manager);

// Periodic maintenance function (should be called regularly)
void uavcanStabilityUpdate(UavcanStabilityManager* manager);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_SYSTEM_STABILITY_H
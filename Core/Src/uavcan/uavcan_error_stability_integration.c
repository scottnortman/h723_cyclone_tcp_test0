#include "uavcan/uavcan_error_handler.h"
#include "uavcan/uavcan_system_stability.h"
#include "uavcan/uavcan_types.h"
#include <stdio.h>

// Global error handler and stability manager instances
static UavcanErrorHandler g_error_handler;
static UavcanStabilityManager g_stability_manager;
static bool g_system_initialized = false;

/**
 * Custom error callback that integrates with stability manager
 */
static void errorCallbackWithStability(const UavcanErrorContext* error_ctx)
{
    if (error_ctx == NULL || !g_system_initialized) {
        return;
    }

    printf("[UAVCAN][ERROR_CALLBACK] %s in %s:%lu - %s\r\n",
           error_ctx->severity >= UAVCAN_LOG_LEVEL_ERROR ? "ERROR" : "WARNING",
           error_ctx->function_name ? error_ctx->function_name : "unknown",
           (unsigned long)error_ctx->line_number,
           error_ctx->description ? error_ctx->description : "No description");

    // Handle error through stability manager
    uavcanStabilityHandleError(&g_stability_manager, error_ctx->error_code);
}

/**
 * Initialize the integrated error handling and stability system
 */
UavcanError uavcanErrorStabilityInit(UavcanLogLevel min_log_level)
{
    UavcanError result;

    // Initialize error handler
    result = uavcanErrorHandlerInit(&g_error_handler, min_log_level);
    if (result != UAVCAN_ERROR_NONE) {
        printf("[UAVCAN][INIT] Failed to initialize error handler: %s\r\n",
               uavcanGetErrorString(result));
        return result;
    }

    // Initialize stability manager
    result = uavcanStabilityInit(&g_stability_manager, &g_error_handler);
    if (result != UAVCAN_ERROR_NONE) {
        printf("[UAVCAN][INIT] Failed to initialize stability manager: %s\r\n",
               uavcanGetErrorString(result));
        uavcanErrorHandlerDeinit(&g_error_handler);
        return result;
    }

    // Set up error callback to integrate with stability manager
    g_error_handler.error_callback = errorCallbackWithStability;

    g_system_initialized = true;

    printf("[UAVCAN][INIT] Error handling and stability system initialized\r\n");
    return UAVCAN_ERROR_NONE;
}

/**
 * Deinitialize the integrated system
 */
void uavcanErrorStabilityDeinit(void)
{
    if (g_system_initialized) {
        printf("[UAVCAN][DEINIT] Shutting down error handling and stability system\r\n");
        
        uavcanStabilityDeinit(&g_stability_manager);
        uavcanErrorHandlerDeinit(&g_error_handler);
        
        g_system_initialized = false;
    }
}

/**
 * Register a UAVCAN task for monitoring
 */
UavcanError uavcanErrorStabilityRegisterTask(void* task_handle, const char* task_name,
                                            uint32_t heartbeat_interval_ms)
{
    if (!g_system_initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    return uavcanStabilityRegisterTask(&g_stability_manager, task_handle, 
                                      task_name, heartbeat_interval_ms);
}

/**
 * Send a task heartbeat
 */
void uavcanErrorStabilityTaskHeartbeat(void* task_handle)
{
    if (g_system_initialized) {
        uavcanStabilityTaskHeartbeat(&g_stability_manager, task_handle);
    }
}

/**
 * Get the current system stability state
 */
UavcanStabilityState uavcanErrorStabilityGetState(void)
{
    if (!g_system_initialized) {
        return UAVCAN_STABILITY_FAILED;
    }

    return uavcanStabilityGetState(&g_stability_manager);
}

/**
 * Check if the UAVCAN system is operational
 */
bool uavcanErrorStabilityIsOperational(void)
{
    if (!g_system_initialized) {
        return false;
    }

    return uavcanStabilityIsOperational(&g_stability_manager);
}

/**
 * Get comprehensive system statistics
 */
typedef struct {
    UavcanErrorStatistics error_stats;
    UavcanStabilityStatistics stability_stats;
    bool system_initialized;
} UavcanSystemStatistics;

const UavcanSystemStatistics* uavcanErrorStabilityGetStatistics(void)
{
    static UavcanSystemStatistics system_stats;
    
    system_stats.system_initialized = g_system_initialized;
    
    if (g_system_initialized) {
        const UavcanErrorStatistics* error_stats = uavcanGetErrorStatistics(&g_error_handler);
        const UavcanStabilityStatistics* stability_stats = uavcanGetErrorStatistics(&g_stability_manager);
        
        if (error_stats != NULL) {
            system_stats.error_stats = *error_stats;
        }
        
        if (stability_stats != NULL) {
            system_stats.stability_stats = *stability_stats;
        }
    } else {
        memset(&system_stats.error_stats, 0, sizeof(UavcanErrorStatistics));
        memset(&system_stats.stability_stats, 0, sizeof(UavcanStabilityStatistics));
    }
    
    return &system_stats;
}

/**
 * Periodic maintenance function - should be called regularly from main loop
 */
void uavcanErrorStabilityUpdate(void)
{
    if (g_system_initialized) {
        uavcanStabilityUpdate(&g_stability_manager);
    }
}

/**
 * Force system recovery attempt
 */
UavcanError uavcanErrorStabilityForceRecovery(void)
{
    if (!g_system_initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    printf("[UAVCAN][RECOVERY] Forcing system recovery attempt\r\n");
    return uavcanStabilityAttemptRecovery(&g_stability_manager);
}

/**
 * Reset all system statistics
 */
void uavcanErrorStabilityResetStatistics(void)
{
    if (g_system_initialized) {
        printf("[UAVCAN][STATS] Resetting system statistics\r\n");
        uavcanResetErrorStatistics(&g_error_handler);
        uavcanStabilityResetStatistics(&g_stability_manager);
    }
}

/**
 * Print comprehensive system status
 */
void uavcanErrorStabilityPrintStatus(void)
{
    if (!g_system_initialized) {
        printf("[UAVCAN][STATUS] System not initialized\r\n");
        return;
    }

    const UavcanSystemStatistics* stats = uavcanErrorStabilityGetStatistics();
    
    printf("\r\n=== UAVCAN System Status ===\r\n");
    printf("System State: %s\r\n", 
           uavcanStabilityIsOperational(&g_stability_manager) ? "OPERATIONAL" : "NON-OPERATIONAL");
    
    printf("\r\nError Statistics:\r\n");
    printf("  Total Errors: %lu\r\n", (unsigned long)stats->error_stats.total_errors);
    printf("  Critical Errors: %lu\r\n", (unsigned long)stats->error_stats.critical_errors);
    printf("  Recovery Attempts: %lu\r\n", (unsigned long)stats->error_stats.recovery_attempts);
    printf("  Successful Recoveries: %lu\r\n", (unsigned long)stats->error_stats.successful_recoveries);
    
    printf("\r\nStability Statistics:\r\n");
    printf("  Current State: %d\r\n", stats->stability_stats.current_state);
    printf("  Isolation Events: %lu\r\n", (unsigned long)stats->stability_stats.isolation_events);
    printf("  Healthy Tasks: %u/%u\r\n", 
           stats->stability_stats.healthy_tasks, stats->stability_stats.total_tasks);
    printf("  Total Uptime: %llu ms\r\n", (unsigned long long)stats->stability_stats.total_uptime_ms);
    printf("  Degraded Time: %llu ms\r\n", (unsigned long long)stats->stability_stats.degraded_time_ms);
    
    if (stats->stability_stats.total_uptime_ms > 0) {
        uint32_t availability = (uint32_t)(((stats->stability_stats.total_uptime_ms - 
                                           stats->stability_stats.degraded_time_ms) * 100) / 
                                          stats->stability_stats.total_uptime_ms);
        printf("  System Availability: %lu%%\r\n", (unsigned long)availability);
    }
    
    printf("=============================\r\n\r\n");
}

/**
 * Example usage function showing how to integrate with UAVCAN tasks
 */
void uavcanErrorStabilityExampleUsage(void)
{
    printf("=== UAVCAN Error Handling and Stability Integration Example ===\r\n");
    
    // Initialize the system
    UavcanError result = uavcanErrorStabilityInit(UAVCAN_LOG_LEVEL_INFO);
    if (result != UAVCAN_ERROR_NONE) {
        printf("Failed to initialize system: %s\r\n", uavcanGetErrorString(result));
        return;
    }
    
    // Register mock tasks (in real implementation, these would be actual FreeRTOS task handles)
    void* node_task = (void*)0x1001;
    void* tx_task = (void*)0x1002;
    void* rx_task = (void*)0x1003;
    
    uavcanErrorStabilityRegisterTask(node_task, "UAVCAN_Node", 1000);
    uavcanErrorStabilityRegisterTask(tx_task, "UAVCAN_TX", 1000);
    uavcanErrorStabilityRegisterTask(rx_task, "UAVCAN_RX", 2000);
    
    // Simulate normal operation
    printf("\r\n--- Simulating Normal Operation ---\r\n");
    for (int i = 0; i < 3; i++) {
        uavcanErrorStabilityTaskHeartbeat(node_task);
        uavcanErrorStabilityTaskHeartbeat(tx_task);
        uavcanErrorStabilityTaskHeartbeat(rx_task);
        uavcanErrorStabilityUpdate();
        
        // Simulate some time passing
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    uavcanErrorStabilityPrintStatus();
    
    // Simulate some errors
    printf("--- Simulating Error Conditions ---\r\n");
    UAVCAN_LOG_WARNING(&g_error_handler, UAVCAN_ERROR_SEND_FAILED, 
                       "Simulated send failure", 0);
    UAVCAN_LOG_ERROR(&g_error_handler, UAVCAN_ERROR_TIMEOUT, 
                     "Simulated timeout", 0);
    
    uavcanErrorStabilityUpdate();
    uavcanErrorStabilityPrintStatus();
    
    // Simulate critical error
    printf("--- Simulating Critical Error ---\r\n");
    UAVCAN_LOG_CRITICAL(&g_error_handler, UAVCAN_ERROR_MEMORY_ALLOCATION,
                        "Simulated memory allocation failure", 0);
    
    uavcanErrorStabilityUpdate();
    uavcanErrorStabilityPrintStatus();
    
    // Attempt recovery
    printf("--- Attempting Recovery ---\r\n");
    result = uavcanErrorStabilityForceRecovery();
    if (result == UAVCAN_ERROR_NONE) {
        printf("Recovery successful\r\n");
    } else {
        printf("Recovery failed: %s\r\n", uavcanGetErrorString(result));
    }
    
    uavcanErrorStabilityPrintStatus();
    
    // Clean up
    uavcanErrorStabilityDeinit();
    
    printf("=== Example Complete ===\r\n");
}
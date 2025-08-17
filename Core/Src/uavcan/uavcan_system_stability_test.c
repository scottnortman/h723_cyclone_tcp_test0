#include "uavcan/uavcan_system_stability.h"
#include "uavcan/uavcan_error_handler.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Mock FreeRTOS functions for testing
static uint32_t mock_tick_count = 0;
uint32_t xTaskGetTickCount(void) { return mock_tick_count; }
void vTaskDelay(uint32_t ticks) { mock_tick_count += ticks; }

// Mock task handles for testing
static void* mock_task_handle_1 = (void*)0x1001;
static void* mock_task_handle_2 = (void*)0x1002;
static void* mock_task_handle_3 = (void*)0x1003;

/**
 * Test stability manager initialization
 */
static void test_stability_init(void)
{
    printf("Testing stability manager initialization...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    // Initialize error handler first
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    
    // Test successful initialization
    UavcanError result = uavcanStabilityInit(&manager, &error_handler);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.current_state == UAVCAN_STABILITY_NORMAL);
    assert(manager.error_handler == &error_handler);
    assert(manager.isolation_enabled == true);
    assert(manager.monitored_task_count == 0);
    
    // Test NULL parameters
    result = uavcanStabilityInit(NULL, &error_handler);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    result = uavcanStabilityInit(&manager, NULL);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ Stability manager initialization tests passed\n");
}

/**
 * Test task registration and monitoring
 */
static void test_task_monitoring(void)
{
    printf("Testing task monitoring...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Test task registration
    UavcanError result = uavcanStabilityRegisterTask(&manager, mock_task_handle_1, 
                                                    "TestTask1", 1000);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.monitored_task_count == 1);
    assert(manager.task_health[0].task_handle == mock_task_handle_1);
    assert(manager.task_health[0].heartbeat_interval_ms == 1000);
    assert(manager.task_health[0].is_healthy == true);
    
    // Test multiple task registration
    result = uavcanStabilityRegisterTask(&manager, mock_task_handle_2, 
                                        "TestTask2", 2000);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.monitored_task_count == 2);
    
    // Test maximum task limit
    uavcanStabilityRegisterTask(&manager, mock_task_handle_3, "TestTask3", 1500);
    uavcanStabilityRegisterTask(&manager, (void*)0x1004, "TestTask4", 1500);
    
    // Should fail on 5th task
    result = uavcanStabilityRegisterTask(&manager, (void*)0x1005, "TestTask5", 1500);
    assert(result == UAVCAN_ERROR_INVALID_CONFIG);
    assert(manager.monitored_task_count == 4);
    
    // Test NULL parameters
    result = uavcanStabilityRegisterTask(NULL, mock_task_handle_1, "Test", 1000);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    result = uavcanStabilityRegisterTask(&manager, NULL, "Test", 1000);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    result = uavcanStabilityRegisterTask(&manager, mock_task_handle_1, NULL, 1000);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ Task monitoring tests passed\n");
}

/**
 * Test task heartbeat functionality
 */
static void test_task_heartbeat(void)
{
    printf("Testing task heartbeat...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Register a task
    uavcanStabilityRegisterTask(&manager, mock_task_handle_1, "TestTask1", 1000);
    
    mock_tick_count = 1000;
    uint32_t initial_time = manager.task_health[0].last_heartbeat_time;
    
    // Send heartbeat
    mock_tick_count = 2000;
    uavcanStabilityTaskHeartbeat(&manager, mock_task_handle_1);
    
    // Verify heartbeat was recorded
    assert(manager.task_health[0].last_heartbeat_time > initial_time);
    assert(manager.task_health[0].missed_heartbeats == 0);
    assert(manager.task_health[0].is_healthy == true);
    
    // Test heartbeat for non-existent task (should not crash)
    uavcanStabilityTaskHeartbeat(&manager, (void*)0x9999);
    
    // Test NULL parameters
    uavcanStabilityTaskHeartbeat(NULL, mock_task_handle_1);
    uavcanStabilityTaskHeartbeat(&manager, NULL);
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ Task heartbeat tests passed\n");
}

/**
 * Test watchdog functionality
 */
static void test_watchdog(void)
{
    printf("Testing watchdog functionality...\n");
    
    UavcanWatchdog watchdog;
    
    // Test watchdog initialization
    mock_tick_count = 1000;
    UavcanError result = uavcanWatchdogInit(&watchdog, 5000);
    assert(result == UAVCAN_ERROR_NONE);
    assert(watchdog.timeout_ms == 5000);
    assert(watchdog.enabled == true);
    assert(watchdog.timeout_count == 0);
    
    // Test watchdog not expired initially
    assert(uavcanWatchdogIsExpired(&watchdog) == false);
    
    // Test watchdog kick
    mock_tick_count = 2000;
    uavcanWatchdogKick(&watchdog);
    assert(watchdog.last_kick_time == 2000);
    
    // Test watchdog not expired after kick
    mock_tick_count = 6000; // 4 seconds after kick
    assert(uavcanWatchdogIsExpired(&watchdog) == false);
    
    // Test watchdog expired
    mock_tick_count = 8000; // 6 seconds after kick (> 5 second timeout)
    assert(uavcanWatchdogIsExpired(&watchdog) == true);
    
    // Test watchdog reset
    uavcanWatchdogReset(&watchdog);
    assert(watchdog.timeout_count == 1);
    assert(watchdog.last_kick_time == 8000);
    
    // Test disabled watchdog
    watchdog.enabled = false;
    assert(uavcanWatchdogIsExpired(&watchdog) == false);
    
    // Test NULL parameters
    result = uavcanWatchdogInit(NULL, 1000);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    result = uavcanWatchdogInit(&watchdog, 0);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    uavcanWatchdogKick(NULL);
    assert(uavcanWatchdogIsExpired(NULL) == false);
    uavcanWatchdogReset(NULL);
    
    printf("✓ Watchdog tests passed\n");
}

/**
 * Test task health checking
 */
static void test_task_health_checking(void)
{
    printf("Testing task health checking...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Register tasks
    uavcanStabilityRegisterTask(&manager, mock_task_handle_1, "TestTask1", 1000);
    uavcanStabilityRegisterTask(&manager, mock_task_handle_2, "TestTask2", 2000);
    
    mock_tick_count = 1000;
    
    // Send initial heartbeats
    uavcanStabilityTaskHeartbeat(&manager, mock_task_handle_1);
    uavcanStabilityTaskHeartbeat(&manager, mock_task_handle_2);
    
    // Advance time but not enough to trigger timeout
    mock_tick_count = 2500;
    uavcanStabilityCheckTaskHealth(&manager);
    
    // Both tasks should still be healthy
    assert(manager.task_health[0].is_healthy == true);
    assert(manager.task_health[1].is_healthy == true);
    assert(manager.current_state == UAVCAN_STABILITY_NORMAL);
    
    // Advance time to trigger timeout for task 1 (2x interval = 2000ms)
    mock_tick_count = 4000; // 3000ms since last heartbeat for task 1
    uavcanStabilityCheckTaskHealth(&manager);
    
    // Task 1 should be unhealthy, system should enter degraded mode
    assert(manager.task_health[0].is_healthy == false);
    assert(manager.task_health[0].missed_heartbeats > 0);
    assert(manager.current_state == UAVCAN_STABILITY_DEGRADED);
    
    // Send heartbeat for task 1 to recover
    uavcanStabilityTaskHeartbeat(&manager, mock_task_handle_1);
    
    // Task should be healthy again
    assert(manager.task_health[0].is_healthy == true);
    assert(manager.task_health[0].missed_heartbeats == 0);
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ Task health checking tests passed\n");
}

/**
 * Test state management
 */
static void test_state_management(void)
{
    printf("Testing state management...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Test initial state
    assert(uavcanStabilityGetState(&manager) == UAVCAN_STABILITY_NORMAL);
    assert(uavcanStabilityIsOperational(&manager) == true);
    
    // Test state transitions
    UavcanError result = uavcanStabilitySetState(&manager, UAVCAN_STABILITY_DEGRADED);
    assert(result == UAVCAN_ERROR_NONE);
    assert(uavcanStabilityGetState(&manager) == UAVCAN_STABILITY_DEGRADED);
    assert(uavcanStabilityIsOperational(&manager) == true);
    
    result = uavcanStabilitySetState(&manager, UAVCAN_STABILITY_ISOLATED);
    assert(result == UAVCAN_ERROR_NONE);
    assert(uavcanStabilityGetState(&manager) == UAVCAN_STABILITY_ISOLATED);
    assert(uavcanStabilityIsOperational(&manager) == false);
    
    result = uavcanStabilitySetState(&manager, UAVCAN_STABILITY_FAILED);
    assert(result == UAVCAN_ERROR_NONE);
    assert(uavcanStabilityGetState(&manager) == UAVCAN_STABILITY_FAILED);
    assert(uavcanStabilityIsOperational(&manager) == false);
    
    // Test NULL parameters
    assert(uavcanStabilityGetState(NULL) == UAVCAN_STABILITY_FAILED);
    assert(uavcanStabilityIsOperational(NULL) == false);
    
    result = uavcanStabilitySetState(NULL, UAVCAN_STABILITY_NORMAL);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ State management tests passed\n");
}

/**
 * Test error handling and isolation
 */
static void test_error_handling_isolation(void)
{
    printf("Testing error handling and isolation...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Test critical error causing immediate isolation
    UavcanError result = uavcanStabilityHandleError(&manager, UAVCAN_ERROR_INIT_FAILED);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.current_state == UAVCAN_STABILITY_ISOLATED);
    assert(manager.isolation_events == 1);
    
    // Reset for next test
    uavcanStabilitySetState(&manager, UAVCAN_STABILITY_NORMAL);
    
    // Test non-critical error causing degraded mode
    result = uavcanStabilityHandleError(&manager, UAVCAN_ERROR_SEND_FAILED);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.current_state == UAVCAN_STABILITY_DEGRADED);
    
    // Test isolation due to error threshold
    manager.failure_threshold = 2;
    uavcanResetErrorStatistics(&error_handler);
    
    // Generate errors to reach threshold
    uavcanLogError(&error_handler, UAVCAN_ERROR_SEND_FAILED, UAVCAN_LOG_LEVEL_ERROR,
                   "test", 1, "Error 1", 0);
    uavcanLogError(&error_handler, UAVCAN_ERROR_TIMEOUT, UAVCAN_LOG_LEVEL_ERROR,
                   "test", 2, "Error 2", 0);
    
    uavcanStabilitySetState(&manager, UAVCAN_STABILITY_NORMAL);
    result = uavcanStabilityHandleError(&manager, UAVCAN_ERROR_RECEIVE_FAILED);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.current_state == UAVCAN_STABILITY_ISOLATED);
    
    // Test NULL parameter
    result = uavcanStabilityHandleError(NULL, UAVCAN_ERROR_SEND_FAILED);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ Error handling and isolation tests passed\n");
}

/**
 * Test recovery functionality
 */
static void test_recovery(void)
{
    printf("Testing recovery functionality...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Set up isolated state
    uavcanStabilitySetState(&manager, UAVCAN_STABILITY_ISOLATED);
    mock_tick_count = 1000;
    manager.last_recovery_attempt = 1000;
    manager.recovery_timeout_ms = 5000;
    
    // Test recovery too soon
    mock_tick_count = 3000; // Only 2 seconds passed
    UavcanError result = uavcanStabilityAttemptRecovery(&manager);
    assert(result == UAVCAN_ERROR_TIMEOUT);
    
    // Test successful recovery
    mock_tick_count = 7000; // 6 seconds passed (> 5 second timeout)
    result = uavcanStabilityAttemptRecovery(&manager);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.current_state == UAVCAN_STABILITY_NORMAL);
    assert(manager.recovery_attempts == 1);
    assert(manager.successful_recoveries == 1);
    
    // Test degraded mode transitions
    result = uavcanStabilityEnterDegradedMode(&manager);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.current_state == UAVCAN_STABILITY_DEGRADED);
    
    result = uavcanStabilityExitDegradedMode(&manager);
    assert(result == UAVCAN_ERROR_NONE);
    assert(manager.current_state == UAVCAN_STABILITY_NORMAL);
    
    // Test NULL parameters
    result = uavcanStabilityAttemptRecovery(NULL);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    result = uavcanStabilityEnterDegradedMode(NULL);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    result = uavcanStabilityExitDegradedMode(NULL);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ Recovery tests passed\n");
}

/**
 * Test statistics functionality
 */
static void test_statistics(void)
{
    printf("Testing statistics functionality...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Register tasks
    uavcanStabilityRegisterTask(&manager, mock_task_handle_1, "TestTask1", 1000);
    uavcanStabilityRegisterTask(&manager, mock_task_handle_2, "TestTask2", 2000);
    
    // Test initial statistics
    const UavcanStabilityStatistics* stats = uavcanStabilityGetStatistics(&manager);
    assert(stats != NULL);
    assert(stats->current_state == UAVCAN_STABILITY_NORMAL);
    assert(stats->isolation_events == 0);
    assert(stats->recovery_attempts == 0);
    assert(stats->successful_recoveries == 0);
    assert(stats->healthy_tasks == 2);
    assert(stats->total_tasks == 2);
    
    // Generate some events
    uavcanStabilityIsolateSubsystem(&manager);
    uavcanStabilityAttemptRecovery(&manager);
    
    // Check updated statistics
    stats = uavcanStabilityGetStatistics(&manager);
    assert(stats->isolation_events == 1);
    assert(stats->recovery_attempts == 1);
    assert(stats->successful_recoveries == 1);
    
    // Test statistics reset
    uavcanStabilityResetStatistics(&manager);
    stats = uavcanStabilityGetStatistics(&manager);
    assert(stats->isolation_events == 0);
    assert(stats->recovery_attempts == 0);
    assert(stats->successful_recoveries == 0);
    
    // Test NULL parameter
    assert(uavcanStabilityGetStatistics(NULL) == NULL);
    uavcanStabilityResetStatistics(NULL); // Should not crash
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ Statistics tests passed\n");
}

/**
 * Test periodic update functionality
 */
static void test_periodic_update(void)
{
    printf("Testing periodic update functionality...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Register a task
    uavcanStabilityRegisterTask(&manager, mock_task_handle_1, "TestTask1", 1000);
    
    mock_tick_count = 1000;
    
    // First update to establish baseline
    uavcanStabilityUpdate(&manager);
    
    // Advance time and update
    mock_tick_count = 3000;
    uavcanStabilityUpdate(&manager);
    
    // Check that uptime was updated
    const UavcanStabilityStatistics* stats = uavcanStabilityGetStatistics(&manager);
    assert(stats->total_uptime_ms > 0);
    
    // Test automatic recovery from isolated state
    uavcanStabilitySetState(&manager, UAVCAN_STABILITY_ISOLATED);
    manager.last_recovery_attempt = 1000;
    manager.recovery_timeout_ms = 1000; // Short timeout for testing
    
    mock_tick_count = 5000; // Enough time for recovery
    uint32_t prev_recovery_attempts = manager.recovery_attempts;
    
    uavcanStabilityUpdate(&manager);
    
    // Should have attempted recovery
    assert(manager.recovery_attempts > prev_recovery_attempts);
    
    // Test NULL parameter
    uavcanStabilityUpdate(NULL); // Should not crash
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ Periodic update tests passed\n");
}

/**
 * Integration test for system stability under error conditions
 */
static void test_system_stability_integration(void)
{
    printf("Testing system stability integration...\n");
    
    UavcanStabilityManager manager;
    UavcanErrorHandler error_handler;
    
    uavcanErrorHandlerInit(&error_handler, UAVCAN_LOG_LEVEL_DEBUG);
    uavcanStabilityInit(&manager, &error_handler);
    
    // Register multiple tasks
    uavcanStabilityRegisterTask(&manager, mock_task_handle_1, "NodeTask", 1000);
    uavcanStabilityRegisterTask(&manager, mock_task_handle_2, "TxTask", 1000);
    uavcanStabilityRegisterTask(&manager, mock_task_handle_3, "RxTask", 2000);
    
    mock_tick_count = 1000;
    
    // Simulate normal operation
    for (int i = 0; i < 5; i++) {
        mock_tick_count += 500;
        uavcanStabilityTaskHeartbeat(&manager, mock_task_handle_1);
        uavcanStabilityTaskHeartbeat(&manager, mock_task_handle_2);
        if (i % 2 == 0) {
            uavcanStabilityTaskHeartbeat(&manager, mock_task_handle_3);
        }
        uavcanStabilityUpdate(&manager);
    }
    
    // System should be in normal state
    assert(manager.current_state == UAVCAN_STABILITY_NORMAL);
    
    // Simulate task failure (no heartbeats)
    mock_tick_count += 5000; // Long time without heartbeats
    uavcanStabilityUpdate(&manager);
    
    // System should enter degraded mode
    assert(manager.current_state == UAVCAN_STABILITY_DEGRADED);
    
    // Simulate critical error
    uavcanStabilityHandleError(&manager, UAVCAN_ERROR_MEMORY_ALLOCATION);
    
    // System should be isolated
    assert(manager.current_state == UAVCAN_STABILITY_ISOLATED);
    
    // Simulate recovery after timeout
    mock_tick_count += manager.recovery_timeout_ms + 1000;
    uavcanStabilityUpdate(&manager);
    
    // System should attempt recovery
    assert(manager.recovery_attempts > 0);
    assert(manager.current_state == UAVCAN_STABILITY_NORMAL);
    
    // Verify statistics
    const UavcanStabilityStatistics* stats = uavcanStabilityGetStatistics(&manager);
    assert(stats->isolation_events > 0);
    assert(stats->recovery_attempts > 0);
    assert(stats->total_uptime_ms > 0);
    
    uavcanStabilityDeinit(&manager);
    
    printf("✓ System stability integration tests passed\n");
}

/**
 * Run all system stability tests
 */
void uavcanSystemStabilityRunTests(void)
{
    printf("=== UAVCAN System Stability Tests ===\n");
    
    test_stability_init();
    test_task_monitoring();
    test_task_heartbeat();
    test_watchdog();
    test_task_health_checking();
    test_state_management();
    test_error_handling_isolation();
    test_recovery();
    test_statistics();
    test_periodic_update();
    test_system_stability_integration();
    
    printf("=== All System Stability Tests Passed ===\n");
}

// Main function for standalone testing
#ifdef UAVCAN_SYSTEM_STABILITY_TEST_STANDALONE
int main(void)
{
    uavcanSystemStabilityRunTests();
    return 0;
}
#endif
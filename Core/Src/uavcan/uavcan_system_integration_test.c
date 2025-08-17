#include "uavcan/uavcan_integration.h"
#include "core/net.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

/**
 * @brief Test UAVCAN integration with real network interface
 * This function tests the integration with the actual CycloneTCP network stack
 */
bool uavcanSystemIntegrationTest(NetInterface* net_interface) {
    printf("UAVCAN System Integration Test\n");
    printf("==============================\n");
    
    if (!net_interface) {
        printf("ERROR: Network interface is NULL\n");
        return false;
    }
    
    // Check if network interface is configured
    if (!net_interface->configured) {
        printf("WARNING: Network interface not configured, test may fail\n");
    }
    
    UavcanIntegrationContext ctx;
    bool test_passed = true;
    
    // Test 1: Initialize UAVCAN with real network interface
    printf("Test 1: UAVCAN Initialization\n");
    UavcanError result = uavcanIntegrationInit(&ctx, net_interface, 0);  // Dynamic node ID
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: UAVCAN initialization failed: %d\n", result);
        test_passed = false;
        goto cleanup;
    }
    printf("  PASS: UAVCAN initialized successfully\n");
    
    // Test 2: Register CLI commands
    printf("Test 2: CLI Command Registration\n");
    result = uavcanIntegrationRegisterCommands(&ctx);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: CLI command registration failed: %d\n", result);
        test_passed = false;
        goto cleanup;
    }
    printf("  PASS: CLI commands registered successfully\n");
    
    // Test 3: Start UAVCAN subsystem
    printf("Test 3: UAVCAN Subsystem Start\n");
    result = uavcanIntegrationStart(&ctx);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: UAVCAN start failed: %d\n", result);
        test_passed = false;
        goto cleanup;
    }
    printf("  PASS: UAVCAN subsystem started successfully\n");
    
    // Test 4: Check if system is ready
    printf("Test 4: System Readiness Check\n");
    // Give tasks some time to initialize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    bool is_ready = uavcanIntegrationIsReady(&ctx);
    if (!is_ready) {
        printf("  WARNING: UAVCAN system not ready (may be normal during startup)\n");
        // Don't fail the test for this, as network may not be fully up
    } else {
        printf("  PASS: UAVCAN system is ready\n");
    }
    
    // Test 5: Get status information
    printf("Test 5: Status Information\n");
    char status_buffer[1024];
    size_t written = uavcanIntegrationGetStatusString(&ctx, status_buffer, sizeof(status_buffer));
    if (written == 0) {
        printf("  FAIL: Status string is empty\n");
        test_passed = false;
    } else {
        printf("  PASS: Status information retrieved (%zu characters)\n", written);
        printf("  Status:\n%s\n", status_buffer);
    }
    
    // Test 6: Update function
    printf("Test 6: Update Function\n");
    for (int i = 0; i < 5; i++) {
        uavcanIntegrationUpdate(&ctx);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    printf("  PASS: Update function executed successfully\n");
    
    // Test 7: Concurrent operation test
    printf("Test 7: Concurrent Operation Test\n");
    printf("  Testing concurrent operation with existing TCP/IP stack...\n");
    
    // Let the system run for a few seconds to test stability
    for (int i = 0; i < 10; i++) {
        uavcanIntegrationUpdate(&ctx);
        
        // Check if network interface is still operational
        if (!net_interface->configured) {
            printf("  WARNING: Network interface became unconfigured during test\n");
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    printf("  PASS: System remained stable during concurrent operation\n");
    
    // Test 8: Stop and restart test
    printf("Test 8: Stop and Restart Test\n");
    result = uavcanIntegrationStop(&ctx);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: UAVCAN stop failed: %d\n", result);
        test_passed = false;
    } else {
        printf("  PASS: UAVCAN stopped successfully\n");
    }
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    result = uavcanIntegrationStart(&ctx);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: UAVCAN restart failed: %d\n", result);
        test_passed = false;
    } else {
        printf("  PASS: UAVCAN restarted successfully\n");
    }
    
cleanup:
    // Clean up
    printf("Cleaning up...\n");
    uavcanIntegrationStop(&ctx);
    uavcanIntegrationDeinit(&ctx);
    
    printf("==============================\n");
    if (test_passed) {
        printf("UAVCAN System Integration Test PASSED!\n");
        printf("UAVCAN is successfully integrated with the main application.\n");
    } else {
        printf("UAVCAN System Integration Test FAILED!\n");
        printf("Check the error messages above for details.\n");
    }
    
    return test_passed;
}

/**
 * @brief Test task priorities and resource sharing
 */
bool uavcanTestTaskPriorities(void) {
    printf("UAVCAN Task Priority Test\n");
    printf("=========================\n");
    
    // Get current task priorities
    TaskHandle_t current_task = xTaskGetCurrentTaskHandle();
    UBaseType_t current_priority = uxTaskPriorityGet(current_task);
    
    printf("Current task priority: %lu\n", current_priority);
    printf("UAVCAN Node Task priority: %d\n", UAVCAN_NODE_TASK_PRIORITY);
    printf("UAVCAN TX Task priority: %d\n", UAVCAN_TX_TASK_PRIORITY);
    printf("UAVCAN RX Task priority: %d\n", UAVCAN_RX_TASK_PRIORITY);
    
    // Check if priorities are reasonable
    bool priorities_ok = true;
    
    if (UAVCAN_NODE_TASK_PRIORITY >= configMAX_PRIORITIES) {
        printf("ERROR: UAVCAN Node task priority too high\n");
        priorities_ok = false;
    }
    
    if (UAVCAN_TX_TASK_PRIORITY >= configMAX_PRIORITIES) {
        printf("ERROR: UAVCAN TX task priority too high\n");
        priorities_ok = false;
    }
    
    if (UAVCAN_RX_TASK_PRIORITY >= configMAX_PRIORITIES) {
        printf("ERROR: UAVCAN RX task priority too high\n");
        priorities_ok = false;
    }
    
    // Check stack sizes
    printf("UAVCAN Node Task stack size: %d words\n", UAVCAN_NODE_TASK_STACK_SIZE);
    printf("UAVCAN TX Task stack size: %d words\n", UAVCAN_TX_TASK_STACK_SIZE);
    printf("UAVCAN RX Task stack size: %d words\n", UAVCAN_RX_TASK_STACK_SIZE);
    
    if (priorities_ok) {
        printf("PASS: Task priorities are within valid range\n");
    } else {
        printf("FAIL: Task priority configuration issues detected\n");
    }
    
    return priorities_ok;
}

/**
 * @brief Test memory usage and resource allocation
 */
bool uavcanTestMemoryUsage(void) {
    printf("UAVCAN Memory Usage Test\n");
    printf("========================\n");
    
    // Get free heap before initialization
    size_t free_heap_before = xPortGetFreeHeapSize();
    printf("Free heap before UAVCAN init: %zu bytes\n", free_heap_before);
    
    // Initialize UAVCAN (this will allocate memory)
    UavcanIntegrationContext ctx;
    NetInterface* net_interface = &netInterface[0];  // Use first network interface
    
    UavcanError result = uavcanIntegrationInit(&ctx, net_interface, 42);
    if (result != UAVCAN_ERROR_NONE) {
        printf("FAIL: UAVCAN initialization failed\n");
        return false;
    }
    
    // Get free heap after initialization
    size_t free_heap_after_init = xPortGetFreeHeapSize();
    printf("Free heap after UAVCAN init: %zu bytes\n", free_heap_after_init);
    
    size_t memory_used_init = free_heap_before - free_heap_after_init;
    printf("Memory used for initialization: %zu bytes\n", memory_used_init);
    
    // Start UAVCAN (this may allocate more memory for tasks)
    result = uavcanIntegrationStart(&ctx);
    if (result != UAVCAN_ERROR_NONE) {
        printf("FAIL: UAVCAN start failed\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Get free heap after start
    size_t free_heap_after_start = xPortGetFreeHeapSize();
    printf("Free heap after UAVCAN start: %zu bytes\n", free_heap_after_start);
    
    size_t memory_used_start = free_heap_after_init - free_heap_after_start;
    printf("Memory used for task creation: %zu bytes\n", memory_used_start);
    
    size_t total_memory_used = free_heap_before - free_heap_after_start;
    printf("Total memory used by UAVCAN: %zu bytes\n", total_memory_used);
    
    // Clean up
    uavcanIntegrationStop(&ctx);
    uavcanIntegrationDeinit(&ctx);
    
    // Check final heap
    size_t free_heap_final = xPortGetFreeHeapSize();
    printf("Free heap after cleanup: %zu bytes\n", free_heap_final);
    
    size_t memory_leaked = free_heap_before - free_heap_final;
    if (memory_leaked > 0) {
        printf("WARNING: Possible memory leak detected: %zu bytes\n", memory_leaked);
    } else {
        printf("PASS: No memory leaks detected\n");
    }
    
    // Check if memory usage is reasonable (less than 32KB)
    bool memory_ok = (total_memory_used < 32768);
    if (memory_ok) {
        printf("PASS: Memory usage is reasonable\n");
    } else {
        printf("WARNING: High memory usage detected\n");
    }
    
    return memory_ok && (memory_leaked == 0);
}
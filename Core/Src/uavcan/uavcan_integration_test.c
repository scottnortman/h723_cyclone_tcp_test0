#include "uavcan/uavcan_integration.h"
#include <stdio.h>
#include <string.h>

// Mock network interface for testing
static NetInterface mock_net_interface = {0};

/**
 * @brief Test UAVCAN integration initialization
 */
static bool testUavcanIntegrationInit(void) {
    printf("Testing UAVCAN integration initialization...\n");
    
    UavcanIntegrationContext ctx;
    
    // Test with valid parameters
    UavcanError result = uavcanIntegrationInit(&ctx, &mock_net_interface, 42);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: Integration init failed with error: %d\n", result);
        return false;
    }
    
    // Verify initialization
    if (!ctx.initialized) {
        printf("  FAIL: Context not marked as initialized\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    if (ctx.node_context.node_id != 42) {
        printf("  FAIL: Node ID not set correctly\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Test with NULL context
    result = uavcanIntegrationInit(NULL, &mock_net_interface, 42);
    if (result != UAVCAN_ERROR_INVALID_PARAMETER) {
        printf("  FAIL: Should reject NULL context\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Test with NULL network interface
    UavcanIntegrationContext ctx2;
    result = uavcanIntegrationInit(&ctx2, NULL, 42);
    if (result != UAVCAN_ERROR_INVALID_PARAMETER) {
        printf("  FAIL: Should reject NULL network interface\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Clean up
    uavcanIntegrationDeinit(&ctx);
    
    printf("  PASS: Integration initialization test passed\n");
    return true;
}

/**
 * @brief Test UAVCAN integration start/stop
 */
static bool testUavcanIntegrationStartStop(void) {
    printf("Testing UAVCAN integration start/stop...\n");
    
    UavcanIntegrationContext ctx;
    
    // Initialize first
    UavcanError result = uavcanIntegrationInit(&ctx, &mock_net_interface, 42);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: Integration init failed\n");
        return false;
    }
    
    // Test start
    result = uavcanIntegrationStart(&ctx);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: Integration start failed with error: %d\n", result);
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    if (!ctx.started) {
        printf("  FAIL: Context not marked as started\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Test stop
    result = uavcanIntegrationStop(&ctx);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: Integration stop failed with error: %d\n", result);
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    if (ctx.started) {
        printf("  FAIL: Context still marked as started after stop\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Clean up
    uavcanIntegrationDeinit(&ctx);
    
    printf("  PASS: Integration start/stop test passed\n");
    return true;
}

/**
 * @brief Test UAVCAN integration status functions
 */
static bool testUavcanIntegrationStatus(void) {
    printf("Testing UAVCAN integration status functions...\n");
    
    UavcanIntegrationContext ctx;
    
    // Test with uninitialized context
    if (uavcanIntegrationIsReady(&ctx)) {
        printf("  FAIL: Should not be ready when uninitialized\n");
        return false;
    }
    
    // Initialize
    UavcanError result = uavcanIntegrationInit(&ctx, &mock_net_interface, 42);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: Integration init failed\n");
        return false;
    }
    
    // Should not be ready until started
    if (uavcanIntegrationIsReady(&ctx)) {
        printf("  FAIL: Should not be ready when not started\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Test status string
    char status_buffer[512];
    size_t written = uavcanIntegrationGetStatusString(&ctx, status_buffer, sizeof(status_buffer));
    if (written == 0) {
        printf("  FAIL: Status string should not be empty\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    if (strstr(status_buffer, "Initialized: Yes") == NULL) {
        printf("  FAIL: Status string should show initialized\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Clean up
    uavcanIntegrationDeinit(&ctx);
    
    printf("  PASS: Integration status test passed\n");
    return true;
}

/**
 * @brief Test UAVCAN integration global context
 */
static bool testUavcanIntegrationGlobalContext(void) {
    printf("Testing UAVCAN integration global context...\n");
    
    // Should be NULL initially
    UavcanIntegrationContext* global_ctx = uavcanIntegrationGetContext();
    if (global_ctx != NULL) {
        printf("  FAIL: Global context should be NULL initially\n");
        return false;
    }
    
    // Initialize a context
    UavcanIntegrationContext ctx;
    UavcanError result = uavcanIntegrationInit(&ctx, &mock_net_interface, 42);
    if (result != UAVCAN_ERROR_NONE) {
        printf("  FAIL: Integration init failed\n");
        return false;
    }
    
    // Global context should now be available
    global_ctx = uavcanIntegrationGetContext();
    if (global_ctx == NULL) {
        printf("  FAIL: Global context should be available after init\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    if (global_ctx->node_context.node_id != 42) {
        printf("  FAIL: Global context node ID mismatch\n");
        uavcanIntegrationDeinit(&ctx);
        return false;
    }
    
    // Clean up
    uavcanIntegrationDeinit(&ctx);
    
    printf("  PASS: Integration global context test passed\n");
    return true;
}

/**
 * @brief Run all UAVCAN integration tests
 */
bool uavcanIntegrationRunTests(void) {
    printf("Running UAVCAN Integration Tests...\n");
    printf("=====================================\n");
    
    bool all_passed = true;
    
    all_passed &= testUavcanIntegrationInit();
    all_passed &= testUavcanIntegrationStartStop();
    all_passed &= testUavcanIntegrationStatus();
    all_passed &= testUavcanIntegrationGlobalContext();
    
    printf("=====================================\n");
    if (all_passed) {
        printf("All UAVCAN integration tests PASSED!\n");
    } else {
        printf("Some UAVCAN integration tests FAILED!\n");
    }
    
    return all_passed;
}
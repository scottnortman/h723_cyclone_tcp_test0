#include "uavcan/uavcan_heartbeat_service.h"
#include "uavcan/uavcan_node.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Mock FreeRTOS functions for testing
#ifndef FREERTOS_H
// Define minimal FreeRTOS types for testing
typedef void* TaskHandle_t;
typedef void* TimerHandle_t;
typedef unsigned long TickType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#define pdPASS          1
#define pdFAIL          0
#define pdMS_TO_TICKS(x) (x)
#define tskIDLE_PRIORITY 0

// Mock FreeRTOS functions
BaseType_t xTaskCreate(void* pvTaskCode, const char* pcName, 
                      unsigned short usStackDepth, void* pvParameters,
                      UBaseType_t uxPriority, TaskHandle_t* pxCreatedTask)
{
    // Mock implementation - just set a dummy handle
    if (pxCreatedTask != NULL) {
        *pxCreatedTask = (TaskHandle_t)0x12345678;
    }
    return pdPASS;
}

void vTaskDelete(TaskHandle_t xTaskToDelete)
{
    // Mock implementation - do nothing
    (void)xTaskToDelete;
}

TickType_t xTaskGetTickCount(void)
{
    return 1000; // Mock tick count
}

void vTaskDelayUntil(TickType_t* pxPreviousWakeTime, const TickType_t xTimeIncrement)
{
    // Mock implementation - do nothing
    (void)pxPreviousWakeTime;
    (void)xTimeIncrement;
}
#endif

// Test helper functions
static void test_heartbeat_init(void);
static void test_heartbeat_interval_validation(void);
static void test_heartbeat_interval_setting(void);
static void test_heartbeat_enable_disable(void);
static void test_heartbeat_message_generation(void);
static void test_heartbeat_status_string(void);
static void test_heartbeat_reset(void);
static void test_heartbeat_error_conditions(void);

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("PASS: %s\n", message); \
            tests_passed++; \
        } else { \
            printf("FAIL: %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

int main(void)
{
    printf("Running UAVCAN Heartbeat Service Tests...\n\n");

    test_heartbeat_init();
    test_heartbeat_interval_validation();
    test_heartbeat_interval_setting();
    test_heartbeat_enable_disable();
    test_heartbeat_message_generation();
    test_heartbeat_status_string();
    test_heartbeat_reset();
    test_heartbeat_error_conditions();

    printf("\nTest Results: %d passed, %d failed\n", tests_passed, tests_failed);
    return (tests_failed == 0) ? 0 : 1;
}

static void test_heartbeat_init(void)
{
    printf("Testing heartbeat initialization...\n");

    UavcanHeartbeatService hb;
    UavcanNodeContext node_ctx;
    
    // Initialize node context
    error_t result = uavcanNodeInit(&node_ctx, 42);
    TEST_ASSERT(result == NO_ERROR, "Node initialization should succeed");

    // Test successful initialization
    result = uavcanHeartbeatInit(&hb, &node_ctx);
    TEST_ASSERT(result == NO_ERROR, "Heartbeat initialization should succeed");
    TEST_ASSERT(hb.interval_ms == UAVCAN_HEARTBEAT_INTERVAL_DEFAULT_MS, 
                "Default interval should be set");
    TEST_ASSERT(hb.enabled == false, "Service should be disabled initially");
    TEST_ASSERT(hb.task_handle == NULL, "Task handle should be NULL initially");
    TEST_ASSERT(hb.node_ctx == &node_ctx, "Node context should be set correctly");

    // Test NULL parameter handling
    result = uavcanHeartbeatInit(NULL, &node_ctx);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "NULL heartbeat service should fail");

    result = uavcanHeartbeatInit(&hb, NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "NULL node context should fail");

    printf("\n");
}

static void test_heartbeat_interval_validation(void)
{
    printf("Testing heartbeat interval validation...\n");

    // Test valid intervals
    TEST_ASSERT(uavcanHeartbeatValidateInterval(100), "100ms should be valid");
    TEST_ASSERT(uavcanHeartbeatValidateInterval(1000), "1000ms should be valid");
    TEST_ASSERT(uavcanHeartbeatValidateInterval(60000), "60000ms should be valid");

    // Test invalid intervals
    TEST_ASSERT(!uavcanHeartbeatValidateInterval(50), "50ms should be invalid (too small)");
    TEST_ASSERT(!uavcanHeartbeatValidateInterval(99), "99ms should be invalid (too small)");
    TEST_ASSERT(!uavcanHeartbeatValidateInterval(60001), "60001ms should be invalid (too large)");
    TEST_ASSERT(!uavcanHeartbeatValidateInterval(100000), "100000ms should be invalid (too large)");

    printf("\n");
}

static void test_heartbeat_interval_setting(void)
{
    printf("Testing heartbeat interval setting...\n");

    UavcanHeartbeatService hb;
    UavcanNodeContext node_ctx;
    
    uavcanNodeInit(&node_ctx, 42);
    uavcanHeartbeatInit(&hb, &node_ctx);

    // Test setting valid interval
    error_t result = uavcanHeartbeatSetInterval(&hb, 2000);
    TEST_ASSERT(result == NO_ERROR, "Setting valid interval should succeed");
    TEST_ASSERT(uavcanHeartbeatGetInterval(&hb) == 2000, "Interval should be updated");

    // Test setting invalid interval
    result = uavcanHeartbeatSetInterval(&hb, 50);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Setting invalid interval should fail");
    TEST_ASSERT(uavcanHeartbeatGetInterval(&hb) == 2000, "Interval should remain unchanged");

    // Test NULL parameter
    result = uavcanHeartbeatSetInterval(NULL, 1000);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "NULL service should fail");

    // Test getter with NULL
    uint32_t interval = uavcanHeartbeatGetInterval(NULL);
    TEST_ASSERT(interval == 0, "NULL service should return 0");

    printf("\n");
}

static void test_heartbeat_enable_disable(void)
{
    printf("Testing heartbeat enable/disable...\n");

    UavcanHeartbeatService hb;
    UavcanNodeContext node_ctx;
    
    uavcanNodeInit(&node_ctx, 42);
    uavcanHeartbeatInit(&hb, &node_ctx);

    // Test initial state
    TEST_ASSERT(!uavcanHeartbeatIsEnabled(&hb), "Service should be disabled initially");

    // Test enabling
    error_t result = uavcanHeartbeatStart(&hb);
    TEST_ASSERT(result == NO_ERROR, "Starting service should succeed");
    TEST_ASSERT(uavcanHeartbeatIsEnabled(&hb), "Service should be enabled after start");

    // Test stopping
    result = uavcanHeartbeatStop(&hb);
    TEST_ASSERT(result == NO_ERROR, "Stopping service should succeed");
    TEST_ASSERT(!uavcanHeartbeatIsEnabled(&hb), "Service should be disabled after stop");

    // Test enable/disable function
    result = uavcanHeartbeatSetEnabled(&hb, true);
    TEST_ASSERT(result == NO_ERROR, "Enabling should succeed");
    TEST_ASSERT(uavcanHeartbeatIsEnabled(&hb), "Service should be enabled");

    result = uavcanHeartbeatSetEnabled(&hb, false);
    TEST_ASSERT(result == NO_ERROR, "Disabling should succeed");
    TEST_ASSERT(!uavcanHeartbeatIsEnabled(&hb), "Service should be disabled");

    // Test NULL parameters
    TEST_ASSERT(!uavcanHeartbeatIsEnabled(NULL), "NULL service should return false");
    
    result = uavcanHeartbeatStart(NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "NULL service start should fail");

    result = uavcanHeartbeatStop(NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "NULL service stop should fail");

    printf("\n");
}

static void test_heartbeat_message_generation(void)
{
    printf("Testing heartbeat message generation...\n");

    UavcanHeartbeatService hb;
    UavcanNodeContext node_ctx;
    
    uavcanNodeInit(&node_ctx, 42);
    uavcanNodeSetHealth(&node_ctx, UAVCAN_NODE_HEALTH_NOMINAL);
    uavcanNodeSetMode(&node_ctx, UAVCAN_NODE_MODE_OPERATIONAL);
    uavcanHeartbeatInit(&hb, &node_ctx);

    // Test sending heartbeat
    error_t result = uavcanHeartbeatSendNow(&hb);
    TEST_ASSERT(result == NO_ERROR, "Sending heartbeat should succeed");

    // Test with NULL parameter
    result = uavcanHeartbeatSendNow(NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "NULL service should fail");

    printf("\n");
}

static void test_heartbeat_status_string(void)
{
    printf("Testing heartbeat status string...\n");

    UavcanHeartbeatService hb;
    UavcanNodeContext node_ctx;
    char buffer[256];
    
    uavcanNodeInit(&node_ctx, 42);
    uavcanHeartbeatInit(&hb, &node_ctx);

    // Test status string generation
    size_t len = uavcanHeartbeatGetStatusString(&hb, buffer, sizeof(buffer));
    TEST_ASSERT(len > 0, "Status string should be generated");
    TEST_ASSERT(len < sizeof(buffer), "Status string should fit in buffer");
    TEST_ASSERT(strstr(buffer, "Enabled: No") != NULL, "Status should show disabled");
    TEST_ASSERT(strstr(buffer, "1000 ms") != NULL, "Status should show default interval");

    // Enable service and test again
    uavcanHeartbeatStart(&hb);
    len = uavcanHeartbeatGetStatusString(&hb, buffer, sizeof(buffer));
    TEST_ASSERT(strstr(buffer, "Enabled: Yes") != NULL, "Status should show enabled");

    // Test with NULL parameters
    len = uavcanHeartbeatGetStatusString(NULL, buffer, sizeof(buffer));
    TEST_ASSERT(len == 0, "NULL service should return 0");

    len = uavcanHeartbeatGetStatusString(&hb, NULL, sizeof(buffer));
    TEST_ASSERT(len == 0, "NULL buffer should return 0");

    len = uavcanHeartbeatGetStatusString(&hb, buffer, 0);
    TEST_ASSERT(len == 0, "Zero buffer size should return 0");

    uavcanHeartbeatStop(&hb);

    printf("\n");
}

static void test_heartbeat_reset(void)
{
    printf("Testing heartbeat reset...\n");

    UavcanHeartbeatService hb;
    UavcanNodeContext node_ctx;
    
    uavcanNodeInit(&node_ctx, 42);
    uavcanHeartbeatInit(&hb, &node_ctx);

    // Modify service state
    uavcanHeartbeatSetInterval(&hb, 2000);
    uavcanHeartbeatStart(&hb);

    // Test reset
    error_t result = uavcanHeartbeatReset(&hb);
    TEST_ASSERT(result == NO_ERROR, "Reset should succeed");
    TEST_ASSERT(hb.interval_ms == UAVCAN_HEARTBEAT_INTERVAL_DEFAULT_MS, 
                "Interval should be reset to default");
    TEST_ASSERT(!hb.enabled, "Service should be disabled after reset");
    TEST_ASSERT(hb.task_handle == NULL, "Task handle should be NULL after reset");
    TEST_ASSERT(hb.node_ctx == &node_ctx, "Node context should remain unchanged");

    // Test with NULL parameter
    result = uavcanHeartbeatReset(NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "NULL service should fail");

    printf("\n");
}

static void test_heartbeat_error_conditions(void)
{
    printf("Testing heartbeat error conditions...\n");

    UavcanHeartbeatService hb;
    UavcanNodeContext node_ctx;
    
    // Test initialization with uninitialized node
    memset(&node_ctx, 0, sizeof(node_ctx));
    error_t result = uavcanHeartbeatInit(&hb, &node_ctx);
    TEST_ASSERT(result == NO_ERROR, "Init should succeed even with uninitialized node");

    // Test starting with uninitialized node
    result = uavcanHeartbeatStart(&hb);
    TEST_ASSERT(result == NO_ERROR, "Start should succeed (mock implementation)");

    // Test double start
    result = uavcanHeartbeatStart(&hb);
    TEST_ASSERT(result == NO_ERROR, "Double start should succeed (no-op)");

    // Test double stop
    uavcanHeartbeatStop(&hb);
    result = uavcanHeartbeatStop(&hb);
    TEST_ASSERT(result == NO_ERROR, "Double stop should succeed (no-op)");

    printf("\n");
}
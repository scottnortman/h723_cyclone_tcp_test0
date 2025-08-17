#include "uavcan/uavcan_tasks.h"
#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_priority_queue.h"
#include "uavcan/uavcan_udp_transport.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test configuration
#define TEST_NODE_ID 42
#define TEST_TIMEOUT_MS 5000

// Test context
static UavcanTaskContext test_task_ctx;
static UavcanNodeContext test_node_ctx;
static UavcanPriorityQueue test_priority_queue;
static UavcanUdpTransport test_udp_transport;

// Test helper functions
static bool waitForTaskState(const UavcanTaskContext* ctx, 
                           UavcanTaskState expected_node_state,
                           UavcanTaskState expected_tx_state,
                           UavcanTaskState expected_rx_state,
                           uint32_t timeout_ms);
static void printTestResult(const char* test_name, bool passed);

/**
 * @brief Test UAVCAN task initialization
 */
static bool testUavcanTasksInit(void) {
    printf("Testing UAVCAN tasks initialization...\n");

    // Test with NULL parameters
    error_t result = uavcanTasksInit(NULL, &test_node_ctx, &test_priority_queue, &test_udp_transport);
    if (result == UAVCAN_ERROR_NONE) {
        printf("  FAIL: Should reject NULL task context\n");
        return false;
    }

    result = uavcanTasksInit(&test_task_ctx, NULL, &test_priority_queue, &test_udp_transport);
    if (result == UAVCAN_ERROR_NONE) {
        printf("  FAIL: Should reject NULL node context\n");
        return false;
    }

    // Initialize node context first
    result = uavcanNodeInit(&test_node_ctx, TEST_NODE_ID);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to initialize node context: %d\n", result);
        return false;
    }

    // Initialize priority queue
    result = uavcanPriorityQueueInit(&test_priority_queue);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to initialize priority queue: %d\n", result);
        return false;
    }

    // Initialize UDP transport
    result = uavcanUdpTransportInit(&test_udp_transport);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to initialize UDP transport: %d\n", result);
        return false;
    }

    // Test successful initialization
    result = uavcanTasksInit(&test_task_ctx, &test_node_ctx, &test_priority_queue, &test_udp_transport);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to initialize tasks: %d\n", result);
        return false;
    }

    // Verify initial state
    UavcanTaskState node_state, tx_state, rx_state;
    result = uavcanTasksGetStates(&test_task_ctx, &node_state, &tx_state, &rx_state);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to get task states: %d\n", result);
        return false;
    }

    if (node_state != UAVCAN_TASK_STATE_STOPPED ||
        tx_state != UAVCAN_TASK_STATE_STOPPED ||
        rx_state != UAVCAN_TASK_STATE_STOPPED) {
        printf("  FAIL: Initial states incorrect (Node: %d, TX: %d, RX: %d)\n", 
               node_state, tx_state, rx_state);
        return false;
    }

    if (uavcanTasksAreRunning(&test_task_ctx)) {
        printf("  FAIL: Tasks should not be running initially\n");
        return false;
    }

    printf("  PASS: Task initialization successful\n");
    return true;
}

/**
 * @brief Test UAVCAN task start/stop
 */
static bool testUavcanTasksStartStop(void) {
    printf("Testing UAVCAN tasks start/stop...\n");

    // Test starting tasks
    error_t result = uavcanTasksStart(&test_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to start tasks: %d\n", result);
        return false;
    }

    // Wait for tasks to start (with timeout)
    if (!waitForTaskState(&test_task_ctx, 
                         UAVCAN_TASK_STATE_RUNNING,
                         UAVCAN_TASK_STATE_RUNNING,
                         UAVCAN_TASK_STATE_RUNNING,
                         TEST_TIMEOUT_MS)) {
        printf("  FAIL: Tasks did not start within timeout\n");
        return false;
    }

    if (!uavcanTasksAreRunning(&test_task_ctx)) {
        printf("  FAIL: Tasks should be running\n");
        return false;
    }

    // Test double start (should succeed without error)
    result = uavcanTasksStart(&test_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Double start should succeed: %d\n", result);
        return false;
    }

    // Test stopping tasks
    result = uavcanTasksStop(&test_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to stop tasks: %d\n", result);
        return false;
    }

    // Wait for tasks to stop (with timeout)
    if (!waitForTaskState(&test_task_ctx, 
                         UAVCAN_TASK_STATE_STOPPED,
                         UAVCAN_TASK_STATE_STOPPED,
                         UAVCAN_TASK_STATE_STOPPED,
                         TEST_TIMEOUT_MS)) {
        printf("  FAIL: Tasks did not stop within timeout\n");
        return false;
    }

    if (uavcanTasksAreRunning(&test_task_ctx)) {
        printf("  FAIL: Tasks should not be running after stop\n");
        return false;
    }

    printf("  PASS: Task start/stop successful\n");
    return true;
}

/**
 * @brief Test UAVCAN task commands
 */
static bool testUavcanTasksCommands(void) {
    printf("Testing UAVCAN task commands...\n");

    // Start tasks first
    error_t result = uavcanTasksStart(&test_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to start tasks for command test: %d\n", result);
        return false;
    }

    // Wait for tasks to start
    if (!waitForTaskState(&test_task_ctx, 
                         UAVCAN_TASK_STATE_RUNNING,
                         UAVCAN_TASK_STATE_RUNNING,
                         UAVCAN_TASK_STATE_RUNNING,
                         TEST_TIMEOUT_MS)) {
        printf("  FAIL: Tasks did not start for command test\n");
        return false;
    }

    // Test health check command
    result = uavcanTasksSendCommand(&test_task_ctx, UAVCAN_TASK_CMD_HEALTH_CHECK, NULL, 0);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to send health check command: %d\n", result);
        return false;
    }

    // Give some time for command processing
    vTaskDelay(pdMS_TO_TICKS(100));

    // Test invalid command with NULL context
    result = uavcanTasksSendCommand(NULL, UAVCAN_TASK_CMD_HEALTH_CHECK, NULL, 0);
    if (result == UAVCAN_ERROR_NONE) {
        printf("  FAIL: Should reject NULL context for command\n");
        return false;
    }

    // Stop tasks
    result = uavcanTasksStop(&test_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to stop tasks after command test: %d\n", result);
        return false;
    }

    printf("  PASS: Task commands successful\n");
    return true;
}

/**
 * @brief Test UAVCAN task statistics
 */
static bool testUavcanTasksStatistics(void) {
    printf("Testing UAVCAN task statistics...\n");

    uint32_t node_cycles, tx_cycles, rx_cycles;

    // Test getting statistics when stopped
    error_t result = uavcanTasksGetStatistics(&test_task_ctx, &node_cycles, &tx_cycles, &rx_cycles);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to get statistics: %d\n", result);
        return false;
    }

    printf("  Initial cycles - Node: %lu, TX: %lu, RX: %lu\n", 
           (unsigned long)node_cycles, (unsigned long)tx_cycles, (unsigned long)rx_cycles);

    // Start tasks and let them run briefly
    result = uavcanTasksStart(&test_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to start tasks for statistics test: %d\n", result);
        return false;
    }

    // Wait for tasks to start and run
    vTaskDelay(pdMS_TO_TICKS(1000)); // Let tasks run for 1 second

    // Get statistics again
    uint32_t new_node_cycles, new_tx_cycles, new_rx_cycles;
    result = uavcanTasksGetStatistics(&test_task_ctx, &new_node_cycles, &new_tx_cycles, &new_rx_cycles);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to get updated statistics: %d\n", result);
        return false;
    }

    printf("  Updated cycles - Node: %lu, TX: %lu, RX: %lu\n", 
           (unsigned long)new_node_cycles, (unsigned long)new_tx_cycles, (unsigned long)new_rx_cycles);

    // Verify cycles have increased
    if (new_node_cycles <= node_cycles || new_tx_cycles <= tx_cycles || new_rx_cycles <= rx_cycles) {
        printf("  FAIL: Task cycles should have increased\n");
        return false;
    }

    // Stop tasks
    result = uavcanTasksStop(&test_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("  FAIL: Failed to stop tasks after statistics test: %d\n", result);
        return false;
    }

    printf("  PASS: Task statistics successful\n");
    return true;
}

/**
 * @brief Test UAVCAN task status string
 */
static bool testUavcanTasksStatusString(void) {
    printf("Testing UAVCAN task status string...\n");

    char status_buffer[512];
    size_t written = uavcanTasksGetStatusString(&test_task_ctx, status_buffer, sizeof(status_buffer));

    if (written == 0) {
        printf("  FAIL: No status string written\n");
        return false;
    }

    if (written >= sizeof(status_buffer)) {
        printf("  FAIL: Status string truncated\n");
        return false;
    }

    printf("  Status string (%zu chars):\n%s\n", written, status_buffer);

    // Test with NULL parameters
    written = uavcanTasksGetStatusString(NULL, status_buffer, sizeof(status_buffer));
    if (written != 0) {
        printf("  FAIL: Should return 0 for NULL context\n");
        return false;
    }

    written = uavcanTasksGetStatusString(&test_task_ctx, NULL, sizeof(status_buffer));
    if (written != 0) {
        printf("  FAIL: Should return 0 for NULL buffer\n");
        return false;
    }

    printf("  PASS: Task status string successful\n");
    return true;
}

/**
 * @brief Wait for specific task states with timeout
 */
static bool waitForTaskState(const UavcanTaskContext* ctx, 
                           UavcanTaskState expected_node_state,
                           UavcanTaskState expected_tx_state,
                           UavcanTaskState expected_rx_state,
                           uint32_t timeout_ms) {
    TickType_t start_time = xTaskGetTickCount();
    TickType_t timeout_ticks = pdMS_TO_TICKS(timeout_ms);

    while ((xTaskGetTickCount() - start_time) < timeout_ticks) {
        UavcanTaskState node_state, tx_state, rx_state;
        error_t result = uavcanTasksGetStates(ctx, &node_state, &tx_state, &rx_state);
        
        if (UAVCAN_SUCCEEDED(result) &&
            node_state == expected_node_state &&
            tx_state == expected_tx_state &&
            rx_state == expected_rx_state) {
            return true;
        }

        vTaskDelay(pdMS_TO_TICKS(50)); // Check every 50ms
    }

    return false;
}

/**
 * @brief Print test result
 */
static void printTestResult(const char* test_name, bool passed) {
    printf("%s: %s\n", test_name, passed ? "PASS" : "FAIL");
}

/**
 * @brief Run all UAVCAN task tests
 */
void uavcanTasksRunTests(void) {
    printf("\n=== UAVCAN Tasks Test Suite ===\n");

    bool all_passed = true;
    bool test_result;

    // Initialize test context
    uavcanTasksReset(&test_task_ctx);

    // Run tests
    test_result = testUavcanTasksInit();
    printTestResult("Task Initialization", test_result);
    all_passed &= test_result;

    test_result = testUavcanTasksStartStop();
    printTestResult("Task Start/Stop", test_result);
    all_passed &= test_result;

    test_result = testUavcanTasksCommands();
    printTestResult("Task Commands", test_result);
    all_passed &= test_result;

    test_result = testUavcanTasksStatistics();
    printTestResult("Task Statistics", test_result);
    all_passed &= test_result;

    test_result = testUavcanTasksStatusString();
    printTestResult("Task Status String", test_result);
    all_passed &= test_result;

    printf("\n=== Test Summary ===\n");
    printf("Overall result: %s\n", all_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    printf("========================\n\n");
}

/**
 * @brief Test task lifecycle management
 */
void uavcanTasksTestLifecycle(void) {
    printf("Testing UAVCAN task lifecycle...\n");

    // Initialize components
    error_t result = uavcanNodeInit(&test_node_ctx, TEST_NODE_ID);
    assert(UAVCAN_SUCCEEDED(result));

    result = uavcanPriorityQueueInit(&test_priority_queue);
    assert(UAVCAN_SUCCEEDED(result));

    result = uavcanUdpTransportInit(&test_udp_transport);
    assert(UAVCAN_SUCCEEDED(result));

    result = uavcanTasksInit(&test_task_ctx, &test_node_ctx, &test_priority_queue, &test_udp_transport);
    assert(UAVCAN_SUCCEEDED(result));

    // Start tasks
    result = uavcanTasksStart(&test_task_ctx);
    assert(UAVCAN_SUCCEEDED(result));

    printf("Tasks started, running for 5 seconds...\n");
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Get final statistics
    uint32_t node_cycles, tx_cycles, rx_cycles;
    result = uavcanTasksGetStatistics(&test_task_ctx, &node_cycles, &tx_cycles, &rx_cycles);
    assert(UAVCAN_SUCCEEDED(result));

    printf("Final statistics - Node: %lu, TX: %lu, RX: %lu cycles\n",
           (unsigned long)node_cycles, (unsigned long)tx_cycles, (unsigned long)rx_cycles);

    // Stop tasks
    result = uavcanTasksStop(&test_task_ctx);
    assert(UAVCAN_SUCCEEDED(result));

    printf("Task lifecycle test completed successfully\n");
}
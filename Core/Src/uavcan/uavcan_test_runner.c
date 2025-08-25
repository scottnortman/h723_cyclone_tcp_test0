/**
 * @file uavcan_test_runner.c
 * @brief UAVCAN Test Runner for validation without hardware
 * 
 * This file implements a test runner that can validate UAVCAN functionality
 * in a simulated environment without requiring actual hardware.
 */

/* Includes ------------------------------------------------------------------*/
#include "uavcan/uavcan_test.h"
#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_transport.h"

#include <string.h>
#include <stdio.h>
#include <assert.h>

/* Private defines -----------------------------------------------------------*/
#define TEST_RUNNER_SUCCESS     0
#define TEST_RUNNER_FAILURE     1

/* Mock network interface structure */
typedef struct {
    bool linkState;
    uint32_t ipAddr;
    char name[16];
} MockNetInterface;

/* Private variables ---------------------------------------------------------*/
static MockNetInterface mock_interface = {
    .linkState = true,
    .ipAddr = 0xC0A80101, // 192.168.1.1
    .name = "mock0"
};

/* Private function prototypes -----------------------------------------------*/
static int runBasicNodeTests(void);
static int runNodeIdTests(void);
static int runMemoryTests(void);
static void printTestSummary(int total_tests, int passed_tests, int failed_tests);

/* Test runner functions -----------------------------------------------------*/

/**
 * @brief Main test runner function
 * @retval int Test result (0 = success, 1 = failure)
 */
int uavcanTestRunnerMain(void)
{
    int total_tests = 0;
    int passed_tests = 0;
    int failed_tests = 0;
    int result;
    
    printf("=== UAVCAN Node Manager Test Runner ===\r\n");
    printf("Running tests in simulation mode...\r\n\r\n");
    
    // Test 1: Basic node operations
    printf("Running basic node tests...\r\n");
    result = runBasicNodeTests();
    total_tests++;
    if (result == TEST_RUNNER_SUCCESS) {
        passed_tests++;
        printf("✓ Basic node tests PASSED\r\n");
    } else {
        failed_tests++;
        printf("✗ Basic node tests FAILED\r\n");
    }
    
    // Test 2: Node ID management
    printf("Running node ID tests...\r\n");
    result = runNodeIdTests();
    total_tests++;
    if (result == TEST_RUNNER_SUCCESS) {
        passed_tests++;
        printf("✓ Node ID tests PASSED\r\n");
    } else {
        failed_tests++;
        printf("✗ Node ID tests FAILED\r\n");
    }
    
    // Test 3: Memory management
    printf("Running memory tests...\r\n");
    result = runMemoryTests();
    total_tests++;
    if (result == TEST_RUNNER_SUCCESS) {
        passed_tests++;
        printf("✓ Memory tests PASSED\r\n");
    } else {
        failed_tests++;
        printf("✗ Memory tests FAILED\r\n");
    }
    
    // Print summary
    printTestSummary(total_tests, passed_tests, failed_tests);
    
    return (failed_tests == 0) ? TEST_RUNNER_SUCCESS : TEST_RUNNER_FAILURE;
}

/**
 * @brief Run basic node operation tests
 * @retval int Test result
 */
static int runBasicNodeTests(void)
{
    UavcanNode test_node;
    UavcanError error;
    
    // Test node initialization with mock interface
    error = uavcanNodeInit(&test_node, (NetInterface*)&mock_interface);
    if (error != UAVCAN_ERROR_NONE) {
        printf("  ERROR: Node initialization failed with error %d\r\n", error);
        return TEST_RUNNER_FAILURE;
    }
    
    // Verify node is initialized
    if (!uavcanNodeIsInitialized(&test_node)) {
        printf("  ERROR: Node not marked as initialized\r\n");
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Verify initial state
    if (test_node.state != UAVCAN_NODE_STATE_OFFLINE) {
        printf("  ERROR: Node initial state incorrect (expected %d, got %d)\r\n", 
               UAVCAN_NODE_STATE_OFFLINE, test_node.state);
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Test node deinitialization
    error = uavcanNodeDeinit(&test_node);
    if (error != UAVCAN_ERROR_NONE) {
        printf("  ERROR: Node deinitialization failed with error %d\r\n", error);
        return TEST_RUNNER_FAILURE;
    }
    
    // Verify node is not initialized after deinit
    if (uavcanNodeIsInitialized(&test_node)) {
        printf("  ERROR: Node still marked as initialized after deinit\r\n");
        return TEST_RUNNER_FAILURE;
    }
    
    return TEST_RUNNER_SUCCESS;
}

/**
 * @brief Run node ID management tests
 * @retval int Test result
 */
static int runNodeIdTests(void)
{
    UavcanNode test_node;
    UavcanError error;
    UdpardNodeID test_id = 42;
    
    // Initialize node
    error = uavcanNodeInit(&test_node, (NetInterface*)&mock_interface);
    if (error != UAVCAN_ERROR_NONE) {
        printf("  ERROR: Node initialization failed\r\n");
        return TEST_RUNNER_FAILURE;
    }
    
    // Test setting valid node ID
    error = uavcanNodeSetNodeId(&test_node, test_id);
    if (error != UAVCAN_ERROR_NONE) {
        printf("  ERROR: Failed to set valid node ID %d, error %d\r\n", test_id, error);
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Verify node ID was set
    if (uavcanNodeGetNodeId(&test_node) != test_id) {
        printf("  ERROR: Node ID not set correctly (expected %d, got %d)\r\n", 
               test_id, uavcanNodeGetNodeId(&test_node));
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Test setting invalid node ID (too high)
    error = uavcanNodeSetNodeId(&test_node, UAVCAN_NODE_ID_MAX + 1);
    if (error == UAVCAN_ERROR_NONE) {
        printf("  ERROR: Invalid high node ID was accepted\r\n");
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Test setting node ID to unset
    error = uavcanNodeSetNodeId(&test_node, UAVCAN_NODE_ID_UNSET);
    if (error != UAVCAN_ERROR_NONE) {
        printf("  ERROR: Failed to unset node ID, error %d\r\n", error);
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Verify node ID was unset
    if (uavcanNodeGetNodeId(&test_node) != UAVCAN_NODE_ID_UNSET) {
        printf("  ERROR: Node ID not unset correctly\r\n");
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Clean up
    uavcanNodeDeinit(&test_node);
    return TEST_RUNNER_SUCCESS;
}

/**
 * @brief Run memory management tests
 * @retval int Test result
 */
static int runMemoryTests(void)
{
    UavcanNode test_node;
    UavcanError error;
    void* ptr1, *ptr2;
    size_t alloc_size = 256;
    
    // Initialize node
    error = uavcanNodeInit(&test_node, (NetInterface*)&mock_interface);
    if (error != UAVCAN_ERROR_NONE) {
        printf("  ERROR: Node initialization failed\r\n");
        return TEST_RUNNER_FAILURE;
    }
    
    // Test memory allocation
    ptr1 = uavcanNodeMemoryAllocate(&test_node, alloc_size);
    if (ptr1 == NULL) {
        printf("  ERROR: Memory allocation failed\r\n");
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Test second allocation
    ptr2 = uavcanNodeMemoryAllocate(&test_node, alloc_size);
    if (ptr2 == NULL) {
        printf("  ERROR: Second memory allocation failed\r\n");
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Verify allocations are different
    if (ptr1 == ptr2) {
        printf("  ERROR: Memory allocations returned same pointer\r\n");
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Test memory deallocation (should not crash)
    uavcanNodeMemoryFree(&test_node, alloc_size, ptr1);
    uavcanNodeMemoryFree(&test_node, alloc_size, ptr2);
    
    // Test allocation with zero size
    ptr1 = uavcanNodeMemoryAllocate(&test_node, 0);
    if (ptr1 != NULL) {
        printf("  ERROR: Zero-size allocation should return NULL\r\n");
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Test allocation with NULL node
    ptr1 = uavcanNodeMemoryAllocate(NULL, alloc_size);
    if (ptr1 != NULL) {
        printf("  ERROR: Allocation with NULL node should return NULL\r\n");
        uavcanNodeDeinit(&test_node);
        return TEST_RUNNER_FAILURE;
    }
    
    // Clean up
    uavcanNodeDeinit(&test_node);
    return TEST_RUNNER_SUCCESS;
}

/**
 * @brief Print test summary
 * @param total_tests Total number of tests
 * @param passed_tests Number of passed tests
 * @param failed_tests Number of failed tests
 */
static void printTestSummary(int total_tests, int passed_tests, int failed_tests)
{
    printf("\r\n=== Test Summary ===\r\n");
    printf("Total Tests: %d\r\n", total_tests);
    printf("Passed: %d\r\n", passed_tests);
    printf("Failed: %d\r\n", failed_tests);
    printf("Success Rate: %.1f%%\r\n", (float)passed_tests / total_tests * 100.0f);
    
    if (failed_tests == 0) {
        printf("🎉 ALL TESTS PASSED!\r\n");
    } else {
        printf("❌ %d TEST(S) FAILED\r\n", failed_tests);
    }
    printf("===================\r\n\r\n");
}

/**
 * @brief Simple test runner that can be called from main or CLI
 * @retval bool True if all tests pass, false otherwise
 */
bool uavcanTestRunnerExecute(void)
{
    return (uavcanTestRunnerMain() == TEST_RUNNER_SUCCESS);
}
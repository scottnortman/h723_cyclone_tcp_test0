#include "uavcan/uavcan_node_id_allocator.h"
#include <stdio.h>
#include <string.h>

// Test configuration
#define TEST_PREFERRED_NODE_ID 50
#define TEST_BUFFER_SIZE 512

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

// Test callback tracking
static uint8_t callback_node_id = 0;
static bool callback_success = false;
static bool callback_called = false;

// Test helper macros
#define TEST_ASSERT(condition, test_name) \
    do { \
        if (condition) { \
            printf("[PASS] %s\n", test_name); \
            tests_passed++; \
        } else { \
            printf("[FAIL] %s\n", test_name); \
            tests_failed++; \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, test_name) \
    TEST_ASSERT((expected) == (actual), test_name)

// Test callback function
static void testAllocationCallback(uint8_t node_id, bool success) {
    callback_node_id = node_id;
    callback_success = success;
    callback_called = true;
}

/**
 * @brief Test allocator initialization
 */
static void test_allocator_init(void) {
    UavcanDynamicNodeIdAllocator allocator;
    error_t result;

    // Test valid initialization
    result = uavcanDynamicNodeIdAllocatorInit(&allocator, TEST_PREFERRED_NODE_ID, testAllocationCallback);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Allocator init with valid parameters");
    TEST_ASSERT_EQUAL(TEST_PREFERRED_NODE_ID, allocator.preferred_node_id, "Preferred node ID set correctly");
    TEST_ASSERT_EQUAL(UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE, allocator.state, "Initial state is idle");
    TEST_ASSERT_EQUAL(false, allocator.allocation_in_progress, "Allocation not in progress initially");

    // Test initialization with NULL allocator
    result = uavcanDynamicNodeIdAllocatorInit(NULL, TEST_PREFERRED_NODE_ID, testAllocationCallback);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Allocator init with NULL allocator");

    // Clean up
    if (allocator.state_mutex != NULL) {
        vSemaphoreDelete(allocator.state_mutex);
    }
}

/**
 * @brief Test allocator start and stop
 */
static void test_allocator_start_stop(void) {
    UavcanDynamicNodeIdAllocator allocator;
    error_t result;

    // Initialize allocator
    result = uavcanDynamicNodeIdAllocatorInit(&allocator, TEST_PREFERRED_NODE_ID, testAllocationCallback);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Allocator init for start/stop test");

    // Test starting allocation
    result = uavcanDynamicNodeIdAllocatorStart(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Start allocation");
    TEST_ASSERT_EQUAL(UAVCAN_DYNAMIC_NODE_ID_STATE_REQUESTING, allocator.state, "State changed to requesting");
    TEST_ASSERT_EQUAL(true, allocator.allocation_in_progress, "Allocation in progress");

    // Test starting already started allocation
    result = uavcanDynamicNodeIdAllocatorStart(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Start already started allocation");

    // Test stopping allocation
    result = uavcanDynamicNodeIdAllocatorStop(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Stop allocation");
    TEST_ASSERT_EQUAL(UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE, allocator.state, "State changed to idle");
    TEST_ASSERT_EQUAL(false, allocator.allocation_in_progress, "Allocation not in progress");

    // Clean up
    if (allocator.state_mutex != NULL) {
        vSemaphoreDelete(allocator.state_mutex);
    }
}

/**
 * @brief Test allocation process
 */
static void test_allocation_process(void) {
    UavcanDynamicNodeIdAllocator allocator;
    error_t result;

    // Reset callback tracking
    callback_called = false;
    callback_success = false;
    callback_node_id = 0;

    // Initialize and start allocator
    result = uavcanDynamicNodeIdAllocatorInit(&allocator, TEST_PREFERRED_NODE_ID, testAllocationCallback);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Allocator init for process test");

    result = uavcanDynamicNodeIdAllocatorStart(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Start allocation for process test");

    // Process allocation (should complete in simulation mode)
    result = uavcanDynamicNodeIdAllocatorProcess(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Process allocation");

    // Check if allocation completed
    bool is_complete = uavcanDynamicNodeIdAllocatorIsComplete(&allocator);
    TEST_ASSERT_EQUAL(true, is_complete, "Allocation completed");

    uint8_t allocated_id = uavcanDynamicNodeIdAllocatorGetAllocatedId(&allocator);
    TEST_ASSERT(allocated_id != UAVCAN_NODE_ID_UNSET, "Valid node ID allocated");
    TEST_ASSERT(uavcanIsValidNodeId(allocated_id), "Allocated ID is valid");

    // Check callback was called
    TEST_ASSERT_EQUAL(true, callback_called, "Callback was called");
    TEST_ASSERT_EQUAL(true, callback_success, "Callback reported success");
    TEST_ASSERT_EQUAL(allocated_id, callback_node_id, "Callback received correct node ID");

    // Clean up
    if (allocator.state_mutex != NULL) {
        vSemaphoreDelete(allocator.state_mutex);
    }
}

/**
 * @brief Test node ID availability checking
 */
static void test_node_id_availability(void) {
    // Test valid node IDs
    bool available = uavcanDynamicNodeIdAllocatorIsIdAvailable(UAVCAN_NODE_ID_MIN);
    TEST_ASSERT_EQUAL(true, available, "Minimum node ID is available");

    available = uavcanDynamicNodeIdAllocatorIsIdAvailable(UAVCAN_NODE_ID_MAX);
    TEST_ASSERT_EQUAL(true, available, "Maximum node ID is available");

    available = uavcanDynamicNodeIdAllocatorIsIdAvailable(TEST_PREFERRED_NODE_ID);
    TEST_ASSERT_EQUAL(true, available, "Preferred node ID is available");

    // Test invalid node IDs
    available = uavcanDynamicNodeIdAllocatorIsIdAvailable(UAVCAN_NODE_ID_UNSET);
    TEST_ASSERT_EQUAL(false, available, "Unset node ID is not available");

    available = uavcanDynamicNodeIdAllocatorIsIdAvailable(UAVCAN_NODE_ID_MAX + 1);
    TEST_ASSERT_EQUAL(false, available, "Invalid high node ID is not available");

    available = uavcanDynamicNodeIdAllocatorIsIdAvailable(0);
    TEST_ASSERT_EQUAL(false, available, "Zero node ID is not available");
}

/**
 * @brief Test allocator state management
 */
static void test_allocator_state_management(void) {
    UavcanDynamicNodeIdAllocator allocator;
    error_t result;

    // Initialize allocator
    result = uavcanDynamicNodeIdAllocatorInit(&allocator, TEST_PREFERRED_NODE_ID, testAllocationCallback);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Allocator init for state test");

    // Test initial state
    UavcanDynamicNodeIdState state = uavcanDynamicNodeIdAllocatorGetState(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE, state, "Initial state is idle");

    // Test state after start
    uavcanDynamicNodeIdAllocatorStart(&allocator);
    state = uavcanDynamicNodeIdAllocatorGetState(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_DYNAMIC_NODE_ID_STATE_REQUESTING, state, "State is requesting after start");

    // Test reset
    uavcanDynamicNodeIdAllocatorReset(&allocator);
    state = uavcanDynamicNodeIdAllocatorGetState(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_DYNAMIC_NODE_ID_STATE_IDLE, state, "State is idle after reset");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_ID_UNSET, allocator.allocated_node_id, "Node ID reset");
    TEST_ASSERT_EQUAL(false, allocator.allocation_in_progress, "Allocation not in progress after reset");

    // Clean up
    if (allocator.state_mutex != NULL) {
        vSemaphoreDelete(allocator.state_mutex);
    }
}

/**
 * @brief Test fallback node ID mechanism
 */
static void test_fallback_node_id(void) {
    UavcanDynamicNodeIdAllocator allocator;
    
    // Initialize allocator
    uavcanDynamicNodeIdAllocatorInit(&allocator, TEST_PREFERRED_NODE_ID, testAllocationCallback);

    // Test fallback ID generation
    uint8_t fallback_id = uavcanDynamicNodeIdAllocatorGetFallbackId(&allocator);
    TEST_ASSERT(fallback_id != UAVCAN_NODE_ID_UNSET, "Fallback ID is valid");
    TEST_ASSERT(uavcanIsValidNodeId(fallback_id), "Fallback ID is in valid range");

    // Clean up
    if (allocator.state_mutex != NULL) {
        vSemaphoreDelete(allocator.state_mutex);
    }
}

/**
 * @brief Test status string generation
 */
static void test_status_string(void) {
    UavcanDynamicNodeIdAllocator allocator;
    char buffer[TEST_BUFFER_SIZE];
    size_t result;

    // Initialize allocator
    uavcanDynamicNodeIdAllocatorInit(&allocator, TEST_PREFERRED_NODE_ID, testAllocationCallback);

    // Test status string generation
    result = uavcanDynamicNodeIdAllocatorGetStatusString(&allocator, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "Status string generated");
    TEST_ASSERT(strstr(buffer, "Dynamic Node ID Allocator Status") != NULL, "Status contains header");
    TEST_ASSERT(strstr(buffer, "State: Idle") != NULL, "Status contains state");
    TEST_ASSERT(strstr(buffer, "Preferred ID: 50") != NULL, "Status contains preferred ID");

    // Test with NULL parameters
    result = uavcanDynamicNodeIdAllocatorGetStatusString(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result, "Status string with NULL allocator");

    result = uavcanDynamicNodeIdAllocatorGetStatusString(&allocator, NULL, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result, "Status string with NULL buffer");

    // Clean up
    if (allocator.state_mutex != NULL) {
        vSemaphoreDelete(allocator.state_mutex);
    }
}

/**
 * @brief Test conflict detection
 */
static void test_conflict_detection(void) {
    UavcanDynamicNodeIdAllocator allocator;
    error_t result;

    // Initialize and start allocator
    uavcanDynamicNodeIdAllocatorInit(&allocator, TEST_PREFERRED_NODE_ID, testAllocationCallback);
    uavcanDynamicNodeIdAllocatorStart(&allocator);
    
    // Process to get an allocated ID
    uavcanDynamicNodeIdAllocatorProcess(&allocator);
    uint8_t allocated_id = uavcanDynamicNodeIdAllocatorGetAllocatedId(&allocator);

    // Simulate conflict detection
    result = uavcanDynamicNodeIdAllocatorDetectConflict(&allocator, allocated_id);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Conflict detection processed");
    
    UavcanDynamicNodeIdState state = uavcanDynamicNodeIdAllocatorGetState(&allocator);
    TEST_ASSERT_EQUAL(UAVCAN_DYNAMIC_NODE_ID_STATE_CONFLICT_DETECTED, state, "State changed to conflict detected");

    // Clean up
    if (allocator.state_mutex != NULL) {
        vSemaphoreDelete(allocator.state_mutex);
    }
}

/**
 * @brief Run all unit tests for dynamic node ID allocator
 */
void uavcanDynamicNodeIdAllocatorRunTests(void) {
    printf("\n=== UAVCAN Dynamic Node ID Allocator Unit Tests ===\n");

    tests_passed = 0;
    tests_failed = 0;

    // Run all test functions
    test_allocator_init();
    test_allocator_start_stop();
    test_allocation_process();
    test_node_id_availability();
    test_allocator_state_management();
    test_fallback_node_id();
    test_status_string();
    test_conflict_detection();

    // Print test summary
    printf("\n=== Test Summary ===\n");
    printf("Tests Passed: %d\n", tests_passed);
    printf("Tests Failed: %d\n", tests_failed);
    printf("Total Tests: %d\n", tests_passed + tests_failed);

    if (tests_failed == 0) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED!\n");
    }
}
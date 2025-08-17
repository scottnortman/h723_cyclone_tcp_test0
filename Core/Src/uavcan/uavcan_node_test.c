#include "uavcan/uavcan_node.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test configuration
#define TEST_NODE_ID 42
#define TEST_BUFFER_SIZE 512

// Test result tracking
static int tests_passed = 0;
static int tests_failed = 0;

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

#define TEST_ASSERT_NOT_NULL(ptr, test_name) \
    TEST_ASSERT((ptr) != NULL, test_name)

/**
 * @brief Test node initialization with valid parameters
 */
static void test_node_init_valid(void) {
    UavcanNodeContext ctx;
    error_t result;

    // Test initialization with valid node ID
    result = uavcanNodeInit(&ctx, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node init with valid ID");
    TEST_ASSERT_EQUAL(TEST_NODE_ID, ctx.node_id, "Node ID set correctly");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_HEALTH_NOMINAL, ctx.health, "Initial health is nominal");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_MODE_INITIALIZATION, ctx.mode, "Initial mode is initialization");
    TEST_ASSERT_EQUAL(false, ctx.initialized, "Node not initialized until started");

    // Test initialization with dynamic node ID
    result = uavcanNodeInit(&ctx, UAVCAN_NODE_ID_UNSET);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node init with dynamic ID");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_ID_UNSET, ctx.node_id, "Dynamic node ID set correctly");
}

/**
 * @brief Test node initialization with invalid parameters
 */
static void test_node_init_invalid(void) {
    UavcanNodeContext ctx;
    error_t result;

    // Test initialization with NULL context
    result = uavcanNodeInit(NULL, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Node init with NULL context");

    // Test initialization with invalid node ID (too high)
    result = uavcanNodeInit(&ctx, UAVCAN_NODE_ID_MAX + 1);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_CONFIG, result, "Node init with invalid high ID");

    // Test initialization with invalid node ID (too low, but not 0)
    // Note: 0 is valid for dynamic allocation, so we can't test this case
}

/**
 * @brief Test node start and stop operations
 */
static void test_node_start_stop(void) {
    UavcanNodeContext ctx;
    error_t result;

    // Initialize node first
    result = uavcanNodeInit(&ctx, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node init for start/stop test");

    // Test starting the node
    result = uavcanNodeStart(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node start");
    TEST_ASSERT_EQUAL(true, ctx.initialized, "Node marked as initialized after start");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_MODE_OPERATIONAL, ctx.mode, "Node mode is operational after start");

    // Test starting already started node
    result = uavcanNodeStart(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node start when already started");

    // Test stopping the node
    result = uavcanNodeStop(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node stop");
    TEST_ASSERT_EQUAL(false, ctx.initialized, "Node marked as not initialized after stop");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_MODE_OFFLINE, ctx.mode, "Node mode is offline after stop");

    // Test stopping already stopped node
    result = uavcanNodeStop(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node stop when already stopped");
}

/**
 * @brief Test node health management
 */
static void test_node_health_management(void) {
    UavcanNodeContext ctx;
    error_t result;
    UavcanNodeHealth health;

    // Initialize node
    result = uavcanNodeInit(&ctx, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node init for health test");

    // Test getting initial health
    health = uavcanNodeGetHealth(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_NODE_HEALTH_NOMINAL, health, "Initial health is nominal");

    // Test setting valid health values
    result = uavcanNodeSetHealth(&ctx, UAVCAN_NODE_HEALTH_ADVISORY);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Set health to advisory");
    health = uavcanNodeGetHealth(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_NODE_HEALTH_ADVISORY, health, "Health changed to advisory");

    result = uavcanNodeSetHealth(&ctx, UAVCAN_NODE_HEALTH_WARNING);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Set health to warning");
    health = uavcanNodeGetHealth(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_NODE_HEALTH_WARNING, health, "Health changed to warning");

    // Test setting invalid health value
    result = uavcanNodeSetHealth(&ctx, (UavcanNodeHealth)255);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Set invalid health value");
}

/**
 * @brief Test node mode management
 */
static void test_node_mode_management(void) {
    UavcanNodeContext ctx;
    error_t result;
    UavcanNodeMode mode;

    // Initialize node
    result = uavcanNodeInit(&ctx, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node init for mode test");

    // Test getting initial mode
    mode = uavcanNodeGetMode(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_NODE_MODE_INITIALIZATION, mode, "Initial mode is initialization");

    // Test setting valid mode values
    result = uavcanNodeSetMode(&ctx, UAVCAN_NODE_MODE_OPERATIONAL);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Set mode to operational");
    mode = uavcanNodeGetMode(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_NODE_MODE_OPERATIONAL, mode, "Mode changed to operational");

    result = uavcanNodeSetMode(&ctx, UAVCAN_NODE_MODE_MAINTENANCE);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Set mode to maintenance");
    mode = uavcanNodeGetMode(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_NODE_MODE_MAINTENANCE, mode, "Mode changed to maintenance");

    // Test setting invalid mode value
    result = uavcanNodeSetMode(&ctx, (UavcanNodeMode)255);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Set invalid mode value");
}

/**
 * @brief Test node ID management
 */
static void test_node_id_management(void) {
    UavcanNodeContext ctx;
    error_t result;
    uint8_t node_id;

    // Initialize node
    result = uavcanNodeInit(&ctx, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node init for ID test");

    // Test getting node ID
    node_id = uavcanNodeGetId(&ctx);
    TEST_ASSERT_EQUAL(TEST_NODE_ID, node_id, "Get node ID");

    // Test setting valid node ID
    result = uavcanNodeSetId(&ctx, 100);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Set valid node ID");
    node_id = uavcanNodeGetId(&ctx);
    TEST_ASSERT_EQUAL(100, node_id, "Node ID changed correctly");

    // Test setting dynamic node ID
    result = uavcanNodeSetId(&ctx, UAVCAN_NODE_ID_UNSET);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Set dynamic node ID");
    node_id = uavcanNodeGetId(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_NODE_ID_UNSET, node_id, "Dynamic node ID set correctly");

    // Test setting invalid node ID
    result = uavcanNodeSetId(&ctx, UAVCAN_NODE_ID_MAX + 1);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_CONFIG, result, "Set invalid node ID");
}

/**
 * @brief Test node configuration validation
 */
static void test_node_config_validation(void) {
    error_t result;

    // Test valid node IDs
    result = uavcanNodeValidateConfig(UAVCAN_NODE_ID_MIN);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Validate minimum node ID");

    result = uavcanNodeValidateConfig(UAVCAN_NODE_ID_MAX);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Validate maximum node ID");

    result = uavcanNodeValidateConfig(UAVCAN_NODE_ID_UNSET);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Validate dynamic node ID");

    // Test invalid node IDs
    result = uavcanNodeValidateConfig(UAVCAN_NODE_ID_MAX + 1);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_CONFIG, result, "Validate invalid high node ID");
}

/**
 * @brief Test node status string generation
 */
static void test_node_status_string(void) {
    UavcanNodeContext ctx;
    char buffer[TEST_BUFFER_SIZE];
    size_t result;

    // Initialize and start node
    uavcanNodeInit(&ctx, TEST_NODE_ID);
    uavcanNodeStart(&ctx);

    // Test status string generation
    result = uavcanNodeGetStatusString(&ctx, buffer, sizeof(buffer));
    TEST_ASSERT(result > 0, "Status string generated");
    TEST_ASSERT(strstr(buffer, "Node ID: 42") != NULL, "Status contains node ID");
    TEST_ASSERT(strstr(buffer, "Status: Running") != NULL, "Status contains running state");
    TEST_ASSERT(strstr(buffer, "Health: Nominal") != NULL, "Status contains health");
    TEST_ASSERT(strstr(buffer, "Mode: Operational") != NULL, "Status contains mode");

    // Test with NULL parameters
    result = uavcanNodeGetStatusString(NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result, "Status string with NULL context");

    result = uavcanNodeGetStatusString(&ctx, NULL, sizeof(buffer));
    TEST_ASSERT_EQUAL(0, result, "Status string with NULL buffer");
}

/**
 * @brief Test node reset functionality
 */
static void test_node_reset(void) {
    UavcanNodeContext ctx;

    // Initialize and modify node
    uavcanNodeInit(&ctx, TEST_NODE_ID);
    uavcanNodeStart(&ctx);
    uavcanNodeSetHealth(&ctx, UAVCAN_NODE_HEALTH_WARNING);

    // Reset the node
    uavcanNodeReset(&ctx);

    // Verify reset state
    TEST_ASSERT_EQUAL(UAVCAN_NODE_ID_UNSET, ctx.node_id, "Node ID reset to unset");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_HEALTH_NOMINAL, ctx.health, "Health reset to nominal");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_MODE_OFFLINE, ctx.mode, "Mode reset to offline");
    TEST_ASSERT_EQUAL(false, ctx.initialized, "Initialized flag reset");
    TEST_ASSERT_EQUAL(0, ctx.uptime_sec, "Uptime reset");
}

/**
 * @brief Test dynamic node ID allocation integration
 */
static void test_dynamic_allocation_integration(void) {
    UavcanNodeContext ctx;
    error_t result;

    // Initialize node with dynamic allocation
    result = uavcanNodeInit(&ctx, UAVCAN_NODE_ID_UNSET);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Node init with dynamic allocation");
    TEST_ASSERT_EQUAL(UAVCAN_NODE_ID_UNSET, ctx.node_id, "Node ID is unset for dynamic allocation");

    // Initialize dynamic allocation
    result = uavcanNodeInitDynamicAllocation(&ctx, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Dynamic allocation init");
    TEST_ASSERT(ctx.dynamic_node_id_allocator != NULL, "Dynamic allocator created");

    // Start dynamic allocation
    result = uavcanNodeStartDynamicAllocation(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Dynamic allocation start");

    // Process dynamic allocation
    result = uavcanNodeProcessDynamicAllocation(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Dynamic allocation process");

    // Check if allocation completed
    bool is_complete = uavcanNodeIsDynamicAllocationComplete(&ctx);
    TEST_ASSERT_EQUAL(true, is_complete, "Dynamic allocation completed");

    uint8_t allocated_id = uavcanNodeGetDynamicAllocatedId(&ctx);
    TEST_ASSERT(allocated_id != UAVCAN_NODE_ID_UNSET, "Valid node ID allocated");
    TEST_ASSERT_EQUAL(allocated_id, ctx.node_id, "Node context updated with allocated ID");

    // Clean up
    if (ctx.dynamic_node_id_allocator != NULL) {
        UAVCAN_FREE(ctx.dynamic_node_id_allocator);
    }
}

/**
 * @brief Test dynamic allocation error cases
 */
static void test_dynamic_allocation_errors(void) {
    UavcanNodeContext ctx;
    error_t result;

    // Test with NULL context
    result = uavcanNodeInitDynamicAllocation(NULL, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Dynamic allocation init with NULL context");

    // Initialize node
    uavcanNodeInit(&ctx, UAVCAN_NODE_ID_UNSET);

    // Test starting without initialization
    result = uavcanNodeStartDynamicAllocation(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INIT_FAILED, result, "Dynamic allocation start without init");

    // Test processing without initialization
    result = uavcanNodeProcessDynamicAllocation(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Dynamic allocation process without init");

    // Test status functions without initialization
    bool is_complete = uavcanNodeIsDynamicAllocationComplete(&ctx);
    TEST_ASSERT_EQUAL(false, is_complete, "Dynamic allocation not complete without init");

    uint8_t allocated_id = uavcanNodeGetDynamicAllocatedId(&ctx);
    TEST_ASSERT_EQUAL(UAVCAN_NODE_ID_UNSET, allocated_id, "No allocated ID without init");
}

/**
 * @brief Run all unit tests for UAVCAN node manager
 */
void uavcanNodeRunTests(void) {
    printf("\n=== UAVCAN Node Manager Unit Tests ===\n");

    tests_passed = 0;
    tests_failed = 0;

    // Run all test functions
    test_node_init_valid();
    test_node_init_invalid();
    test_node_start_stop();
    test_node_health_management();
    test_node_mode_management();
    test_node_id_management();
    test_node_config_validation();
    test_node_status_string();
    test_node_reset();
    test_dynamic_allocation_integration();
    test_dynamic_allocation_errors();

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
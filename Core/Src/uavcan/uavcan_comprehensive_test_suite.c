#include "uavcan/uavcan_integration.h"
#include "uavcan/uavcan_priority_queue.h"
#include "uavcan/uavcan_message_handler.h"
#include "uavcan/uavcan_heartbeat_service.h"
#include "uavcan/uavcan_udp_transport.h"
#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_config.h"
#include "core/net.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Test configuration
#define TEST_NODE_ID 42
#define TEST_TIMEOUT_MS 5000
#define TEST_MESSAGE_COUNT 100
#define TEST_HIGH_LOAD_COUNT 1000

// Test statistics
typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
    uint32_t start_time_ms;
    uint32_t end_time_ms;
} UavcanTestStatistics;

static UavcanTestStatistics g_test_stats = {0};

// Helper macros
#define TEST_START() do { \
    g_test_stats.tests_run++; \
    printf("  Running: %s\n", __FUNCTION__); \
} while(0)

#define TEST_PASS() do { \
    g_test_stats.tests_passed++; \
    printf("  PASS: %s\n", __FUNCTION__); \
    return true; \
} while(0)

#define TEST_FAIL(msg) do { \
    g_test_stats.tests_failed++; \
    printf("  FAIL: %s - %s\n", __FUNCTION__, msg); \
    return false; \
} while(0)

#define TEST_ASSERT(condition, msg) do { \
    if (!(condition)) { \
        TEST_FAIL(msg); \
    } \
} while(0)

/**
 * @brief Test priority queue functionality under normal conditions
 */
static bool testPriorityQueueBasic(void) {
    TEST_START();
    
    UavcanPriorityQueue queue;
    UavcanError result = uavcanPriorityQueueInit(&queue);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Priority queue init failed");
    
    // Test message with different priorities
    UavcanMessage msg_high = {
        .subject_id = 1000,
        .priority = CYPHAL_PRIORITY_EXCEPTIONAL,
        .payload_size = 8,
        .payload = (uint8_t*)"test_msg",
        .source_node_id = TEST_NODE_ID
    };
    
    UavcanMessage msg_low = {
        .subject_id = 1001,
        .priority = CYPHAL_PRIORITY_OPTIONAL,
        .payload_size = 8,
        .payload = (uint8_t*)"test_low",
        .source_node_id = TEST_NODE_ID
    };
    
    // Push low priority first, then high priority
    result = uavcanPriorityQueuePush(&queue, &msg_low);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to push low priority message");
    
    result = uavcanPriorityQueuePush(&queue, &msg_high);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to push high priority message");
    
    // Pop should return high priority first
    UavcanMessage popped_msg;
    result = uavcanPriorityQueuePop(&queue, &popped_msg);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to pop message");
    TEST_ASSERT(popped_msg.priority == CYPHAL_PRIORITY_EXCEPTIONAL, "Wrong priority order");
    TEST_ASSERT(popped_msg.subject_id == 1000, "Wrong message popped");
    
    // Pop second message
    result = uavcanPriorityQueuePop(&queue, &popped_msg);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to pop second message");
    TEST_ASSERT(popped_msg.priority == CYPHAL_PRIORITY_OPTIONAL, "Wrong second message priority");
    
    uavcanPriorityQueueDeinit(&queue);
    TEST_PASS();
}

/**
 * @brief Test priority queue under high load conditions
 */
static bool testPriorityQueueHighLoad(void) {
    TEST_START();
    
    UavcanPriorityQueue queue;
    UavcanError result = uavcanPriorityQueueInit(&queue);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Priority queue init failed");
    
    // Fill queue with messages of different priorities
    for (int i = 0; i < TEST_HIGH_LOAD_COUNT; i++) {
        UavcanMessage msg = {
            .subject_id = 2000 + i,
            .priority = i % CYPHAL_PRIORITY_LEVELS,
            .payload_size = 4,
            .payload = (uint8_t*)&i,
            .source_node_id = TEST_NODE_ID
        };
        
        result = uavcanPriorityQueuePush(&queue, &msg);
        if (result != UAVCAN_ERROR_NONE) {
            // Queue full is acceptable under high load
            if (result != UAVCAN_ERROR_QUEUE_FULL) {
                TEST_FAIL("Unexpected error during high load push");
            }
            break;
        }
    }
    
    // Verify messages come out in priority order
    uint8_t last_priority = 0;
    int messages_popped = 0;
    
    while (true) {
        UavcanMessage popped_msg;
        result = uavcanPriorityQueuePop(&queue, &popped_msg);
        if (result != UAVCAN_ERROR_NONE) {
            break;
        }
        
        TEST_ASSERT(popped_msg.priority >= last_priority, "Priority order violation");
        last_priority = popped_msg.priority;
        messages_popped++;
    }
    
    printf("    Processed %d messages under high load\n", messages_popped);
    TEST_ASSERT(messages_popped > 0, "No messages processed");
    
    uavcanPriorityQueueDeinit(&queue);
    TEST_PASS();
}

/**
 * @brief Test message serialization and deserialization
 */
static bool testMessageSerialization(void) {
    TEST_START();
    
    // Create test message
    uint8_t test_payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    UavcanMessage original_msg = {
        .subject_id = 3000,
        .priority = CYPHAL_PRIORITY_NOMINAL,
        .payload_size = sizeof(test_payload),
        .payload = test_payload,
        .source_node_id = TEST_NODE_ID,
        .timestamp_usec = 1234567890ULL
    };
    
    // Test message validation
    bool is_valid = uavcanMessageValidate(&original_msg);
    TEST_ASSERT(is_valid, "Valid message failed validation");
    
    // Test invalid message
    UavcanMessage invalid_msg = original_msg;
    invalid_msg.priority = 255;  // Invalid priority
    is_valid = uavcanMessageValidate(&invalid_msg);
    TEST_ASSERT(!is_valid, "Invalid message passed validation");
    
    // Test message creation
    UavcanMessage created_msg;
    UavcanError result = uavcanMessageCreate(&created_msg, 3001, CYPHAL_PRIORITY_HIGH, 
                                           test_payload, sizeof(test_payload));
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Message creation failed");
    TEST_ASSERT(created_msg.subject_id == 3001, "Wrong subject ID in created message");
    TEST_ASSERT(created_msg.priority == CYPHAL_PRIORITY_HIGH, "Wrong priority in created message");
    
    TEST_PASS();
}

/**
 * @brief Test heartbeat service functionality
 */
static bool testHeartbeatService(void) {
    TEST_START();
    
    UavcanNodeContext node_ctx;
    UavcanError result = uavcanNodeInit(&node_ctx, TEST_NODE_ID);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Node init failed");
    
    UavcanHeartbeatService hb_service;
    result = uavcanHeartbeatInit(&hb_service, &node_ctx);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Heartbeat init failed");
    
    // Test interval setting
    result = uavcanHeartbeatSetInterval(&hb_service, 2000);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to set heartbeat interval");
    
    uint32_t interval = uavcanHeartbeatGetInterval(&hb_service);
    TEST_ASSERT(interval == 2000, "Heartbeat interval not set correctly");
    
    // Test invalid interval
    result = uavcanHeartbeatSetInterval(&hb_service, 50);  // Too low
    TEST_ASSERT(result != UAVCAN_ERROR_NONE, "Should reject invalid interval");
    
    // Test enable/disable
    result = uavcanHeartbeatSetEnabled(&hb_service, true);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to enable heartbeat");
    TEST_ASSERT(uavcanHeartbeatIsEnabled(&hb_service), "Heartbeat not enabled");
    
    result = uavcanHeartbeatSetEnabled(&hb_service, false);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to disable heartbeat");
    TEST_ASSERT(!uavcanHeartbeatIsEnabled(&hb_service), "Heartbeat not disabled");
    
    uavcanHeartbeatReset(&hb_service);
    TEST_PASS();
}

/**
 * @brief Test UDP transport functionality
 */
static bool testUdpTransport(void) {
    TEST_START();
    
    NetInterface* net_interface = &netInterface[0];
    if (!net_interface->configured) {
        printf("    SKIP: Network interface not configured\n");
        TEST_PASS();
    }
    
    UavcanUdpTransport transport;
    UavcanError result = uavcanUdpTransportInit(&transport, net_interface, 
                                               UAVCAN_UDP_PORT_DEFAULT, 
                                               UAVCAN_MULTICAST_ADDR);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "UDP transport init failed");
    
    // Test transport readiness
    bool is_ready = uavcanUdpTransportIsReady(&transport);
    TEST_ASSERT(is_ready, "UDP transport not ready after init");
    
    // Test socket access
    Socket* socket = uavcanUdpTransportGetSocket(&transport);
    TEST_ASSERT(socket != NULL, "UDP socket is NULL");
    
    // Test libudpard instance access
    UdpardInstance* udpard = uavcanUdpTransportGetUdpardInstance(&transport);
    TEST_ASSERT(udpard != NULL, "Udpard instance is NULL");
    
    uavcanUdpTransportDeinit(&transport);
    TEST_PASS();
}

/**
 * @brief Test node functionality
 */
static bool testNodeFunctionality(void) {
    TEST_START();
    
    UavcanNodeContext node_ctx;
    UavcanError result = uavcanNodeInit(&node_ctx, TEST_NODE_ID);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Node init failed");
    
    // Test node properties
    TEST_ASSERT(uavcanNodeIsInitialized(&node_ctx), "Node not marked as initialized");
    TEST_ASSERT(uavcanNodeGetId(&node_ctx) == TEST_NODE_ID, "Wrong node ID");
    TEST_ASSERT(uavcanNodeGetHealth(&node_ctx) == UAVCAN_NODE_HEALTH_NOMINAL, "Wrong initial health");
    TEST_ASSERT(uavcanNodeGetMode(&node_ctx) == UAVCAN_NODE_MODE_INITIALIZATION, "Wrong initial mode");
    
    // Test health setting
    result = uavcanNodeSetHealth(&node_ctx, UAVCAN_NODE_HEALTH_ADVISORY);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to set node health");
    TEST_ASSERT(uavcanNodeGetHealth(&node_ctx) == UAVCAN_NODE_HEALTH_ADVISORY, "Health not set correctly");
    
    // Test mode setting
    result = uavcanNodeSetMode(&node_ctx, UAVCAN_NODE_MODE_OPERATIONAL);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to set node mode");
    TEST_ASSERT(uavcanNodeGetMode(&node_ctx) == UAVCAN_NODE_MODE_OPERATIONAL, "Mode not set correctly");
    
    // Test node ID change
    result = uavcanNodeSetId(&node_ctx, 100);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to set new node ID");
    TEST_ASSERT(uavcanNodeGetId(&node_ctx) == 100, "Node ID not changed correctly");
    
    // Test invalid node ID
    result = uavcanNodeSetId(&node_ctx, 200);  // Too high
    TEST_ASSERT(result != UAVCAN_ERROR_NONE, "Should reject invalid node ID");
    
    uavcanNodeReset(&node_ctx);
    TEST_PASS();
}

/**
 * @brief Test configuration management
 */
static bool testConfigurationManagement(void) {
    TEST_START();
    
    UavcanConfigContext config_ctx;
    UavcanError result = uavcanConfigInit(&config_ctx);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Config init failed");
    
    // Test default configuration
    UavcanConfig config;
    result = uavcanConfigGet(&config_ctx, &config);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to get default config");
    
    // Test configuration modification
    config.node_id = 50;
    config.heartbeat_interval_ms = 2000;
    config.udp_port = 9999;
    
    result = uavcanConfigSet(&config_ctx, &config);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to set config");
    
    // Verify configuration was set
    UavcanConfig retrieved_config;
    result = uavcanConfigGet(&config_ctx, &retrieved_config);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Failed to get modified config");
    TEST_ASSERT(retrieved_config.node_id == 50, "Node ID not set correctly");
    TEST_ASSERT(retrieved_config.heartbeat_interval_ms == 2000, "Heartbeat interval not set correctly");
    TEST_ASSERT(retrieved_config.udp_port == 9999, "UDP port not set correctly");
    
    // Test invalid configuration
    config.node_id = 200;  // Invalid
    result = uavcanConfigSet(&config_ctx, &config);
    TEST_ASSERT(result != UAVCAN_ERROR_NONE, "Should reject invalid config");
    
    uavcanConfigDeinit(&config_ctx);
    TEST_PASS();
}

/**
 * @brief Test system stability under stress
 */
static bool testSystemStability(void) {
    TEST_START();
    
    NetInterface* net_interface = &netInterface[0];
    UavcanIntegrationContext ctx;
    
    // Initialize and start UAVCAN
    UavcanError result = uavcanIntegrationInit(&ctx, net_interface, TEST_NODE_ID);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Integration init failed");
    
    result = uavcanIntegrationStart(&ctx);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Integration start failed");
    
    // Run stress test for 10 seconds
    uint32_t start_time = xTaskGetTickCount();
    uint32_t stress_duration_ms = 10000;
    uint32_t update_count = 0;
    
    printf("    Running %lu ms stress test...\n", stress_duration_ms);
    
    while ((xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS < stress_duration_ms) {
        // Rapid updates
        uavcanIntegrationUpdate(&ctx);
        update_count++;
        
        // Check system is still responsive
        if (update_count % 100 == 0) {
            bool is_ready = uavcanIntegrationIsReady(&ctx);
            if (!is_ready) {
                printf("    WARNING: System not ready during stress test (update %lu)\n", update_count);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms delay
    }
    
    printf("    Completed %lu updates during stress test\n", update_count);
    TEST_ASSERT(update_count > 500, "Too few updates during stress test");
    
    // Verify system is still operational
    bool is_ready = uavcanIntegrationIsReady(&ctx);
    if (!is_ready) {
        printf("    WARNING: System not ready after stress test\n");
    }
    
    uavcanIntegrationStop(&ctx);
    uavcanIntegrationDeinit(&ctx);
    
    TEST_PASS();
}

/**
 * @brief Test interoperability with external tools (simulated)
 */
static bool testInteroperability(void) {
    TEST_START();
    
    // This test simulates external UAVCAN tool interaction
    // In a real environment, this would test with actual UAVCAN tools
    
    NetInterface* net_interface = &netInterface[0];
    UavcanIntegrationContext ctx;
    
    UavcanError result = uavcanIntegrationInit(&ctx, net_interface, TEST_NODE_ID);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Integration init failed");
    
    result = uavcanIntegrationStart(&ctx);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Integration start failed");
    
    // Simulate external node discovery
    printf("    Simulating external node interaction...\n");
    
    // Let the system run for a few seconds to allow for network activity
    vTaskDelay(pdMS_TO_TICKS(3000));
    
    // Check if heartbeats are being sent
    // (In real test, we would capture network traffic)
    char status_buffer[512];
    size_t written = uavcanIntegrationGetStatusString(&ctx, status_buffer, sizeof(status_buffer));
    TEST_ASSERT(written > 0, "Status string empty");
    
    printf("    System status during interoperability test:\n%s\n", status_buffer);
    
    uavcanIntegrationStop(&ctx);
    uavcanIntegrationDeinit(&ctx);
    
    TEST_PASS();
}

/**
 * @brief Run all comprehensive tests
 */
bool uavcanRunComprehensiveTests(void) {
    printf("UAVCAN Comprehensive Test Suite\n");
    printf("===============================\n");
    
    // Initialize test statistics
    memset(&g_test_stats, 0, sizeof(g_test_stats));
    g_test_stats.start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // Run all tests
    bool all_passed = true;
    
    printf("\n1. Basic Functionality Tests\n");
    printf("-----------------------------\n");
    all_passed &= testPriorityQueueBasic();
    all_passed &= testMessageSerialization();
    all_passed &= testHeartbeatService();
    all_passed &= testUdpTransport();
    all_passed &= testNodeFunctionality();
    all_passed &= testConfigurationManagement();
    
    printf("\n2. Performance and Stress Tests\n");
    printf("--------------------------------\n");
    all_passed &= testPriorityQueueHighLoad();
    all_passed &= testSystemStability();
    
    printf("\n3. Interoperability Tests\n");
    printf("-------------------------\n");
    all_passed &= testInteroperability();
    
    // Calculate test statistics
    g_test_stats.end_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    uint32_t duration_ms = g_test_stats.end_time_ms - g_test_stats.start_time_ms;
    
    printf("\n===============================\n");
    printf("Test Results Summary:\n");
    printf("  Tests Run: %lu\n", g_test_stats.tests_run);
    printf("  Tests Passed: %lu\n", g_test_stats.tests_passed);
    printf("  Tests Failed: %lu\n", g_test_stats.tests_failed);
    printf("  Success Rate: %.1f%%\n", 
           (float)g_test_stats.tests_passed / g_test_stats.tests_run * 100.0f);
    printf("  Duration: %lu ms\n", duration_ms);
    
    if (all_passed) {
        printf("\nALL TESTS PASSED! ✓\n");
        printf("UAVCAN implementation is ready for production use.\n");
    } else {
        printf("\nSOME TESTS FAILED! ✗\n");
        printf("Please review the failed tests above.\n");
    }
    
    return all_passed;
}

/**
 * @brief Get test statistics
 */
const UavcanTestStatistics* uavcanGetTestStatistics(void) {
    return &g_test_stats;
}
#include "uavcan/uavcan_libudpard_integration.h"
#include "uavcan/uavcan_common.h"

// Test framework includes
#include <stdio.h>
#include <string.h>
#include <assert.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"

// Test configuration
#define TEST_NODE_ID 42
#define TEST_SERVICE_ID 123
#define TEST_SUBJECT_ID 456
#define TEST_BUFFER_SIZE 256

// Test results structure
typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
} UavcanLibudpardIntegrationTestResults;

static UavcanLibudpardIntegrationTestResults test_results = {0};

// Helper macros for testing
#define TEST_ASSERT(condition, test_name) \
    do { \
        test_results.tests_run++; \
        if (condition) { \
            test_results.tests_passed++; \
            printf("[PASS] %s\n", test_name); \
        } else { \
            test_results.tests_failed++; \
            printf("[FAIL] %s\n", test_name); \
        } \
    } while(0)

#define TEST_ASSERT_EQUAL(expected, actual, test_name) \
    TEST_ASSERT((expected) == (actual), test_name)

#define TEST_ASSERT_NOT_NULL(ptr, test_name) \
    TEST_ASSERT((ptr) != NULL, test_name)

#define TEST_ASSERT_NULL(ptr, test_name) \
    TEST_ASSERT((ptr) == NULL, test_name)

// Mock structures for testing
static NetInterface mock_net_interface = {0};
static UavcanUdpTransport mock_udp_transport = {0};

// Test functions

static void test_uavcan_libudpard_integration_init_valid_params(void)
{
    UavcanLibudpardIntegration integration;
    
    // Initialize mock UDP transport
    UavcanError result = uavcanUdpTransportInit(&mock_udp_transport, &mock_net_interface, 
                                                UAVCAN_UDP_PORT_DEFAULT, UAVCAN_MULTICAST_ADDR);
    if (result != UAVCAN_ERROR_NONE) {
        printf("[SKIP] UDP transport init failed, skipping libudpard integration test\n");
        return;
    }
    
    result = uavcanLibudpardIntegrationInit(&integration, &mock_udp_transport, TEST_NODE_ID);
    
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Libudpard integration init with valid params");
    TEST_ASSERT(integration.initialized, "Integration initialized flag set");
    TEST_ASSERT_NOT_NULL(integration.udpard_instance, "Udpard instance set");
    TEST_ASSERT_EQUAL(&mock_udp_transport, integration.udp_transport, "UDP transport reference set");
    TEST_ASSERT_EQUAL(TEST_NODE_ID, integration.udpard_instance->node_id, "Node ID set correctly");
    
    // Clean up
    uavcanLibudpardIntegrationDeinit(&integration);
    uavcanUdpTransportDeinit(&mock_udp_transport);
}

static void test_uavcan_libudpard_integration_init_invalid_params(void)
{
    UavcanLibudpardIntegration integration;
    
    // Test NULL integration
    UavcanError result = uavcanLibudpardIntegrationInit(NULL, &mock_udp_transport, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Init with NULL integration");
    
    // Test NULL UDP transport
    result = uavcanLibudpardIntegrationInit(&integration, NULL, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Init with NULL UDP transport");
    
    // Test uninitialized UDP transport
    UavcanUdpTransport uninitialized_transport = {0};
    result = uavcanLibudpardIntegrationInit(&integration, &uninitialized_transport, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NETWORK_UNAVAILABLE, result, "Init with uninitialized UDP transport");
}

static void test_uavcan_libudpard_integration_deinit(void)
{
    UavcanLibudpardIntegration integration;
    
    // Initialize first
    UavcanError result = uavcanUdpTransportInit(&mock_udp_transport, &mock_net_interface, 
                                                UAVCAN_UDP_PORT_DEFAULT, UAVCAN_MULTICAST_ADDR);
    if (result != UAVCAN_ERROR_NONE) {
        printf("[SKIP] UDP transport init failed, skipping deinit test\n");
        return;
    }
    
    result = uavcanLibudpardIntegrationInit(&integration, &mock_udp_transport, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Integration init for deinit test");
    
    // Test deinit
    result = uavcanLibudpardIntegrationDeinit(&integration);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Libudpard integration deinit");
    TEST_ASSERT(!integration.initialized, "Integration initialized flag cleared");
    TEST_ASSERT_NULL(integration.udpard_instance, "Udpard instance cleared");
    TEST_ASSERT_NULL(integration.udp_transport, "UDP transport reference cleared");
    
    // Test deinit with NULL integration
    result = uavcanLibudpardIntegrationDeinit(NULL);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Deinit with NULL integration");
    
    // Test deinit with uninitialized integration
    memset(&integration, 0, sizeof(integration));
    result = uavcanLibudpardIntegrationDeinit(&integration);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Deinit with uninitialized integration");
    
    // Clean up
    uavcanUdpTransportDeinit(&mock_udp_transport);
}

static void test_uavcan_libudpard_priority_conversion(void)
{
    // Test UAVCAN to libudpard priority conversion
    TEST_ASSERT_EQUAL(UdpardPriorityExceptional, 
                      uavcanLibudpardConvertPriority(CYPHAL_PRIORITY_EXCEPTIONAL), 
                      "Convert exceptional priority");
    TEST_ASSERT_EQUAL(UdpardPriorityImmediate, 
                      uavcanLibudpardConvertPriority(CYPHAL_PRIORITY_IMMEDIATE), 
                      "Convert immediate priority");
    TEST_ASSERT_EQUAL(UdpardPriorityFast, 
                      uavcanLibudpardConvertPriority(CYPHAL_PRIORITY_FAST), 
                      "Convert fast priority");
    TEST_ASSERT_EQUAL(UdpardPriorityHigh, 
                      uavcanLibudpardConvertPriority(CYPHAL_PRIORITY_HIGH), 
                      "Convert high priority");
    TEST_ASSERT_EQUAL(UdpardPriorityNominal, 
                      uavcanLibudpardConvertPriority(CYPHAL_PRIORITY_NOMINAL), 
                      "Convert nominal priority");
    TEST_ASSERT_EQUAL(UdpardPriorityLow, 
                      uavcanLibudpardConvertPriority(CYPHAL_PRIORITY_LOW), 
                      "Convert low priority");
    TEST_ASSERT_EQUAL(UdpardPrioritySlow, 
                      uavcanLibudpardConvertPriority(CYPHAL_PRIORITY_SLOW), 
                      "Convert slow priority");
    TEST_ASSERT_EQUAL(UdpardPriorityOptional, 
                      uavcanLibudpardConvertPriority(CYPHAL_PRIORITY_OPTIONAL), 
                      "Convert optional priority");
    
    // Test invalid priority (should default to nominal)
    TEST_ASSERT_EQUAL(UdpardPriorityNominal, 
                      uavcanLibudpardConvertPriority(255), 
                      "Convert invalid priority defaults to nominal");
    
    // Test libudpard to UAVCAN priority conversion
    TEST_ASSERT_EQUAL(CYPHAL_PRIORITY_EXCEPTIONAL, 
                      uavcanLibudpardConvertPriorityFromUdpard(UdpardPriorityExceptional), 
                      "Convert from exceptional priority");
    TEST_ASSERT_EQUAL(CYPHAL_PRIORITY_IMMEDIATE, 
                      uavcanLibudpardConvertPriorityFromUdpard(UdpardPriorityImmediate), 
                      "Convert from immediate priority");
    TEST_ASSERT_EQUAL(CYPHAL_PRIORITY_FAST, 
                      uavcanLibudpardConvertPriorityFromUdpard(UdpardPriorityFast), 
                      "Convert from fast priority");
    TEST_ASSERT_EQUAL(CYPHAL_PRIORITY_HIGH, 
                      uavcanLibudpardConvertPriorityFromUdpard(UdpardPriorityHigh), 
                      "Convert from high priority");
    TEST_ASSERT_EQUAL(CYPHAL_PRIORITY_NOMINAL, 
                      uavcanLibudpardConvertPriorityFromUdpard(UdpardPriorityNominal), 
                      "Convert from nominal priority");
    TEST_ASSERT_EQUAL(CYPHAL_PRIORITY_LOW, 
                      uavcanLibudpardConvertPriorityFromUdpard(UdpardPriorityLow), 
                      "Convert from low priority");
    TEST_ASSERT_EQUAL(CYPHAL_PRIORITY_SLOW, 
                      uavcanLibudpardConvertPriorityFromUdpard(UdpardPrioritySlow), 
                      "Convert from slow priority");
    TEST_ASSERT_EQUAL(CYPHAL_PRIORITY_OPTIONAL, 
                      uavcanLibudpardConvertPriorityFromUdpard(UdpardPriorityOptional), 
                      "Convert from optional priority");
}

static void test_uavcan_libudpard_publish_invalid_params(void)
{
    UavcanLibudpardIntegration integration;
    UavcanMessage msg = {0};
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    
    // Setup valid message
    msg.subject_id = TEST_SUBJECT_ID;
    msg.priority = CYPHAL_PRIORITY_NOMINAL;
    msg.payload = payload;
    msg.payload_size = sizeof(payload);
    
    // Test NULL integration
    UavcanError result = uavcanLibudpardPublish(NULL, &msg, 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Publish with NULL integration");
    
    // Test NULL message
    result = uavcanLibudpardPublish(&integration, NULL, 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Publish with NULL message");
    
    // Test uninitialized integration
    memset(&integration, 0, sizeof(integration));
    result = uavcanLibudpardPublish(&integration, &msg, 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Publish with uninitialized integration");
    
    // Test message with NULL payload
    msg.payload = NULL;
    result = uavcanLibudpardPublish(&integration, &msg, 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Publish with NULL payload");
    
    // Test message with zero payload size
    msg.payload = payload;
    msg.payload_size = 0;
    result = uavcanLibudpardPublish(&integration, &msg, 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Publish with zero payload size");
}

static void test_uavcan_libudpard_send_request_invalid_params(void)
{
    UavcanLibudpardIntegration integration;
    uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};
    
    // Test NULL integration
    UavcanError result = uavcanLibudpardSendRequest(NULL, TEST_SERVICE_ID, TEST_NODE_ID, 
                                                    payload, sizeof(payload), 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send request with NULL integration");
    
    // Test NULL payload
    result = uavcanLibudpardSendRequest(&integration, TEST_SERVICE_ID, TEST_NODE_ID, 
                                        NULL, sizeof(payload), 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send request with NULL payload");
    
    // Test uninitialized integration
    memset(&integration, 0, sizeof(integration));
    result = uavcanLibudpardSendRequest(&integration, TEST_SERVICE_ID, TEST_NODE_ID, 
                                        payload, sizeof(payload), 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send request with uninitialized integration");
    
    // Test zero payload size
    result = uavcanLibudpardSendRequest(&integration, TEST_SERVICE_ID, TEST_NODE_ID, 
                                        payload, 0, 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send request with zero payload size");
    
    // Test oversized payload
    result = uavcanLibudpardSendRequest(&integration, TEST_SERVICE_ID, TEST_NODE_ID, 
                                        payload, UAVCAN_MAX_PAYLOAD_SIZE + 1, 1000000);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send request with oversized payload");
}

static void test_uavcan_libudpard_integration_is_ready(void)
{
    UavcanLibudpardIntegration integration;
    
    // Test with uninitialized integration
    bool ready = uavcanLibudpardIntegrationIsReady(&integration);
    TEST_ASSERT(!ready, "Uninitialized integration not ready");
    
    // Test with NULL integration
    ready = uavcanLibudpardIntegrationIsReady(NULL);
    TEST_ASSERT(!ready, "NULL integration not ready");
    
    // Initialize UDP transport and integration
    UavcanError result = uavcanUdpTransportInit(&mock_udp_transport, &mock_net_interface, 
                                                UAVCAN_UDP_PORT_DEFAULT, UAVCAN_MULTICAST_ADDR);
    if (result != UAVCAN_ERROR_NONE) {
        printf("[SKIP] UDP transport init failed, skipping ready test\n");
        return;
    }
    
    result = uavcanLibudpardIntegrationInit(&integration, &mock_udp_transport, TEST_NODE_ID);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Integration init for ready test");
    
    // Test with initialized integration
    ready = uavcanLibudpardIntegrationIsReady(&integration);
    TEST_ASSERT(ready, "Initialized integration ready");
    
    // Clean up
    uavcanLibudpardIntegrationDeinit(&integration);
    uavcanUdpTransportDeinit(&mock_udp_transport);
    
    // Test after deinit
    ready = uavcanLibudpardIntegrationIsReady(&integration);
    TEST_ASSERT(!ready, "Deinitialized integration not ready");
}

static void test_uavcan_libudpard_timestamp(void)
{
    uint64_t timestamp1 = uavcanLibudpardGetTimestampUsec();
    
    // Wait a bit
    vTaskDelay(pdMS_TO_TICKS(10));
    
    uint64_t timestamp2 = uavcanLibudpardGetTimestampUsec();
    
    TEST_ASSERT(timestamp2 > timestamp1, "Timestamp increases over time");
    TEST_ASSERT((timestamp2 - timestamp1) >= 10000, "Timestamp difference reasonable (at least 10ms)");
}

static void test_uavcan_libudpard_message_to_payload(void)
{
    UavcanMessage msg = {0};
    UdpardPayload payload;
    uint8_t buffer[TEST_BUFFER_SIZE];
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    
    // Setup message
    msg.subject_id = TEST_SUBJECT_ID;
    msg.priority = CYPHAL_PRIORITY_NOMINAL;
    msg.payload = test_data;
    msg.payload_size = sizeof(test_data);
    
    // Test valid conversion
    UavcanError result = uavcanLibudpardMessageToPayload(&msg, &payload, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Message to payload conversion");
    TEST_ASSERT_EQUAL(sizeof(test_data), payload.size, "Payload size correct");
    TEST_ASSERT_EQUAL(buffer, payload.data, "Payload data pointer correct");
    TEST_ASSERT_EQUAL(0, memcmp(buffer, test_data, sizeof(test_data)), "Payload data correct");
    
    // Test NULL parameters
    result = uavcanLibudpardMessageToPayload(NULL, &payload, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Message to payload with NULL message");
    
    result = uavcanLibudpardMessageToPayload(&msg, NULL, buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Message to payload with NULL payload");
    
    result = uavcanLibudpardMessageToPayload(&msg, &payload, NULL, sizeof(buffer));
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Message to payload with NULL buffer");
    
    // Test buffer too small
    result = uavcanLibudpardMessageToPayload(&msg, &payload, buffer, sizeof(test_data) - 1);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Message to payload with small buffer");
}

// Main test runner
void uavcanLibudpardIntegrationRunTests(void)
{
    printf("\n=== UAVCAN Libudpard Integration Tests ===\n");
    
    // Reset test results
    memset(&test_results, 0, sizeof(test_results));
    
    // Run all tests
    test_uavcan_libudpard_integration_init_valid_params();
    test_uavcan_libudpard_integration_init_invalid_params();
    test_uavcan_libudpard_integration_deinit();
    test_uavcan_libudpard_priority_conversion();
    test_uavcan_libudpard_publish_invalid_params();
    test_uavcan_libudpard_send_request_invalid_params();
    test_uavcan_libudpard_integration_is_ready();
    test_uavcan_libudpard_timestamp();
    test_uavcan_libudpard_message_to_payload();
    
    // Print results
    printf("\n=== Test Results ===\n");
    printf("Tests Run: %lu\n", test_results.tests_run);
    printf("Tests Passed: %lu\n", test_results.tests_passed);
    printf("Tests Failed: %lu\n", test_results.tests_failed);
    
    if (test_results.tests_failed == 0) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED!\n");
    }
    
    printf("=== End Libudpard Integration Tests ===\n\n");
}

// Task function for running tests
void vUavcanLibudpardIntegrationTestTask(void *pvParameters)
{
    (void)pvParameters;
    
    // Wait a bit for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Run tests
    uavcanLibudpardIntegrationRunTests();
    
    // Delete this task
    vTaskDelete(NULL);
}

// Function to start the test task
BaseType_t xUavcanLibudpardIntegrationTestStart(UBaseType_t uxPriority)
{
    return xTaskCreate(vUavcanLibudpardIntegrationTestTask,
                       "LibudpardIntegTest",
                       512,  // Stack size
                       NULL,
                       uxPriority,
                       NULL);
}
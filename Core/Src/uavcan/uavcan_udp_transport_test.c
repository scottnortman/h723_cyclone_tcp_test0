#include "uavcan/uavcan_udp_transport.h"
#include "uavcan/uavcan_common.h"

// Test framework includes
#include <stdio.h>
#include <string.h>
#include <assert.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"

// Test configuration
#define TEST_UDP_PORT 9382
#define TEST_MULTICAST_ADDR "239.65.65.65"
#define TEST_BUFFER_SIZE 256

// Test results structure
typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
} UavcanUdpTransportTestResults;

static UavcanUdpTransportTestResults test_results = {0};

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

// Mock network interface (for testing without actual network)
static NetInterface mock_net_interface = {0};

// Test functions

static void test_uavcan_udp_transport_init_valid_params(void)
{
    UavcanUdpTransport transport;
    
    UavcanError result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "UDP transport init with valid params");
    TEST_ASSERT(transport.initialized, "Transport initialized flag set");
    TEST_ASSERT_EQUAL(TEST_UDP_PORT, transport.port, "Port set correctly");
    TEST_ASSERT_NOT_NULL(transport.socket_mutex, "Socket mutex created");
    TEST_ASSERT_NOT_NULL(transport.udp_socket, "UDP socket created");
    
    // Clean up
    uavcanUdpTransportDeinit(&transport);
}

static void test_uavcan_udp_transport_init_invalid_params(void)
{
    UavcanUdpTransport transport;
    
    // Test NULL transport
    UavcanError result = uavcanUdpTransportInit(NULL, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Init with NULL transport");
    
    // Test NULL network interface
    result = uavcanUdpTransportInit(&transport, NULL, 
                                    TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Init with NULL network interface");
    
    // Test NULL multicast address
    result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                    TEST_UDP_PORT, NULL);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Init with NULL multicast address");
    
    // Test invalid multicast address
    result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                    TEST_UDP_PORT, "invalid.address");
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Init with invalid multicast address");
}

static void test_uavcan_udp_transport_deinit(void)
{
    UavcanUdpTransport transport;
    
    // Initialize first
    UavcanError result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Transport init for deinit test");
    
    // Test deinit
    result = uavcanUdpTransportDeinit(&transport);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "UDP transport deinit");
    TEST_ASSERT(!transport.initialized, "Transport initialized flag cleared");
    TEST_ASSERT_NULL(transport.udp_socket, "UDP socket cleared");
    TEST_ASSERT_NULL(transport.socket_mutex, "Socket mutex cleared");
    
    // Test deinit with NULL transport
    result = uavcanUdpTransportDeinit(NULL);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Deinit with NULL transport");
    
    // Test deinit with uninitialized transport
    memset(&transport, 0, sizeof(transport));
    result = uavcanUdpTransportDeinit(&transport);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Deinit with uninitialized transport");
}

static void test_uavcan_udp_transport_send_invalid_params(void)
{
    UavcanUdpTransport transport;
    uint8_t test_data[] = {0x01, 0x02, 0x03, 0x04};
    
    // Initialize transport
    UavcanError result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Transport init for send test");
    
    // Test NULL transport
    result = uavcanUdpTransportSend(NULL, test_data, sizeof(test_data), NULL, 0);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send with NULL transport");
    
    // Test NULL data
    result = uavcanUdpTransportSend(&transport, NULL, sizeof(test_data), NULL, 0);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send with NULL data");
    
    // Test zero size
    result = uavcanUdpTransportSend(&transport, test_data, 0, NULL, 0);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send with zero size");
    
    // Test oversized data
    result = uavcanUdpTransportSend(&transport, test_data, 
                                    UAVCAN_UDP_TRANSPORT_MAX_PAYLOAD_SIZE + 1, NULL, 0);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Send with oversized data");
    
    // Clean up
    uavcanUdpTransportDeinit(&transport);
}

static void test_uavcan_udp_transport_receive_invalid_params(void)
{
    UavcanUdpTransport transport;
    uint8_t buffer[TEST_BUFFER_SIZE];
    size_t received_size;
    
    // Initialize transport
    UavcanError result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Transport init for receive test");
    
    // Test NULL transport
    result = uavcanUdpTransportReceive(NULL, buffer, sizeof(buffer), 
                                       &received_size, NULL, NULL, 100);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Receive with NULL transport");
    
    // Test NULL buffer
    result = uavcanUdpTransportReceive(&transport, NULL, sizeof(buffer), 
                                       &received_size, NULL, NULL, 100);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Receive with NULL buffer");
    
    // Test NULL received_size
    result = uavcanUdpTransportReceive(&transport, buffer, sizeof(buffer), 
                                       NULL, NULL, NULL, 100);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_INVALID_PARAMETER, result, "Receive with NULL received_size");
    
    // Clean up
    uavcanUdpTransportDeinit(&transport);
}

static void test_uavcan_udp_transport_is_ready(void)
{
    UavcanUdpTransport transport;
    
    // Test with uninitialized transport
    bool ready = uavcanUdpTransportIsReady(&transport);
    TEST_ASSERT(!ready, "Uninitialized transport not ready");
    
    // Test with NULL transport
    ready = uavcanUdpTransportIsReady(NULL);
    TEST_ASSERT(!ready, "NULL transport not ready");
    
    // Initialize transport
    UavcanError result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Transport init for ready test");
    
    // Test with initialized transport
    ready = uavcanUdpTransportIsReady(&transport);
    TEST_ASSERT(ready, "Initialized transport ready");
    
    // Clean up
    uavcanUdpTransportDeinit(&transport);
    
    // Test after deinit
    ready = uavcanUdpTransportIsReady(&transport);
    TEST_ASSERT(!ready, "Deinitialized transport not ready");
}

static void test_uavcan_udp_transport_get_socket(void)
{
    UavcanUdpTransport transport;
    
    // Test with uninitialized transport
    Socket* socket = uavcanUdpTransportGetSocket(&transport);
    TEST_ASSERT_NULL(socket, "Uninitialized transport returns NULL socket");
    
    // Test with NULL transport
    socket = uavcanUdpTransportGetSocket(NULL);
    TEST_ASSERT_NULL(socket, "NULL transport returns NULL socket");
    
    // Initialize transport
    UavcanError result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Transport init for socket test");
    
    // Test with initialized transport
    socket = uavcanUdpTransportGetSocket(&transport);
    TEST_ASSERT_NOT_NULL(socket, "Initialized transport returns valid socket");
    TEST_ASSERT_EQUAL(transport.udp_socket, socket, "Returned socket matches internal socket");
    
    // Clean up
    uavcanUdpTransportDeinit(&transport);
}

static void test_uavcan_udp_transport_get_udpard_instance(void)
{
    UavcanUdpTransport transport;
    
    // Test with uninitialized transport
    UdpardInstance* instance = uavcanUdpTransportGetUdpardInstance(&transport);
    TEST_ASSERT_NULL(instance, "Uninitialized transport returns NULL udpard instance");
    
    // Test with NULL transport
    instance = uavcanUdpTransportGetUdpardInstance(NULL);
    TEST_ASSERT_NULL(instance, "NULL transport returns NULL udpard instance");
    
    // Initialize transport
    UavcanError result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Transport init for udpard test");
    
    // Test with initialized transport
    instance = uavcanUdpTransportGetUdpardInstance(&transport);
    TEST_ASSERT_NOT_NULL(instance, "Initialized transport returns valid udpard instance");
    TEST_ASSERT_EQUAL(&transport.udpard_instance, instance, "Returned instance matches internal instance");
    
    // Clean up
    uavcanUdpTransportDeinit(&transport);
}

static void test_uavcan_udp_transport_thread_safety(void)
{
    // This test would require multiple tasks to properly test thread safety
    // For now, we'll just verify that the mutex is created and used
    UavcanUdpTransport transport;
    
    UavcanError result = uavcanUdpTransportInit(&transport, &mock_net_interface, 
                                                TEST_UDP_PORT, TEST_MULTICAST_ADDR);
    TEST_ASSERT_EQUAL(UAVCAN_ERROR_NONE, result, "Transport init for thread safety test");
    TEST_ASSERT_NOT_NULL(transport.socket_mutex, "Socket mutex created for thread safety");
    
    // Clean up
    uavcanUdpTransportDeinit(&transport);
}

// Main test runner
void uavcanUdpTransportRunTests(void)
{
    printf("\n=== UAVCAN UDP Transport Tests ===\n");
    
    // Reset test results
    memset(&test_results, 0, sizeof(test_results));
    
    // Run all tests
    test_uavcan_udp_transport_init_valid_params();
    test_uavcan_udp_transport_init_invalid_params();
    test_uavcan_udp_transport_deinit();
    test_uavcan_udp_transport_send_invalid_params();
    test_uavcan_udp_transport_receive_invalid_params();
    test_uavcan_udp_transport_is_ready();
    test_uavcan_udp_transport_get_socket();
    test_uavcan_udp_transport_get_udpard_instance();
    test_uavcan_udp_transport_thread_safety();
    
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
    
    printf("=== End UDP Transport Tests ===\n\n");
}

// Task function for running tests
void vUavcanUdpTransportTestTask(void *pvParameters)
{
    (void)pvParameters;
    
    // Wait a bit for system to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    // Run tests
    uavcanUdpTransportRunTests();
    
    // Delete this task
    vTaskDelete(NULL);
}

// Function to start the test task
BaseType_t xUavcanUdpTransportTestStart(UBaseType_t uxPriority)
{
    return xTaskCreate(vUavcanUdpTransportTestTask,
                       "UDPTransportTest",
                       512,  // Stack size
                       NULL,
                       uxPriority,
                       NULL);
}
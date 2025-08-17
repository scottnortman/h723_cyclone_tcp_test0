#include "uavcan/uavcan_message_handler.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test counter
static int tests_run = 0;
static int tests_passed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        tests_run++; \
        if (condition) { \
            tests_passed++; \
            printf("PASS: %s\n", message); \
        } else { \
            printf("FAIL: %s\n", message); \
        } \
    } while(0)

// Test data
static const uint8_t test_payload[] = {0x01, 0x02, 0x03, 0x04, 0x05};
static const size_t test_payload_size = sizeof(test_payload);

/**
 * @brief Test message initialization
 */
static void test_uavcan_message_init(void)
{
    printf("\n=== Testing uavcanMessageInit ===\n");
    
    UavcanMessage msg;
    
    // Test successful initialization
    error_t result = uavcanMessageInit(&msg);
    TEST_ASSERT(result == NO_ERROR, "Message initialization should succeed");
    TEST_ASSERT(msg.subject_id == 0, "Subject ID should be initialized to 0");
    TEST_ASSERT(msg.priority == CYPHAL_PRIORITY_NOMINAL, "Priority should be initialized to nominal");
    TEST_ASSERT(msg.payload_size == 0, "Payload size should be initialized to 0");
    TEST_ASSERT(msg.payload == NULL, "Payload pointer should be initialized to NULL");
    TEST_ASSERT(msg.source_node_id == UAVCAN_NODE_ID_UNSET, "Source node ID should be unset");
    TEST_ASSERT(msg.destination_node_id == UAVCAN_NODE_ID_UNSET, "Destination node ID should be unset");
    TEST_ASSERT(msg.is_service_request == false, "Service request flag should be false");
    TEST_ASSERT(msg.is_anonymous == false, "Anonymous flag should be false");
    
    // Test null pointer
    result = uavcanMessageInit(NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null pointer should return invalid parameter error");
}

/**
 * @brief Test message creation
 */
static void test_uavcan_message_create(void)
{
    printf("\n=== Testing uavcanMessageCreate ===\n");
    
    UavcanMessage msg;
    
    // Test successful creation with payload
    error_t result = uavcanMessageCreate(&msg, 100, CYPHAL_PRIORITY_HIGH, test_payload, test_payload_size);
    TEST_ASSERT(result == NO_ERROR, "Message creation with payload should succeed");
    TEST_ASSERT(msg.subject_id == 100, "Subject ID should be set correctly");
    TEST_ASSERT(msg.priority == CYPHAL_PRIORITY_HIGH, "Priority should be set correctly");
    TEST_ASSERT(msg.payload_size == test_payload_size, "Payload size should be set correctly");
    TEST_ASSERT(msg.payload != NULL, "Payload should be allocated");
    TEST_ASSERT(memcmp(msg.payload, test_payload, test_payload_size) == 0, "Payload data should be copied correctly");
    TEST_ASSERT(msg.timestamp_usec > 0, "Timestamp should be set");
    
    uavcanMessageDestroy(&msg);
    
    // Test creation without payload
    result = uavcanMessageCreate(&msg, 200, CYPHAL_PRIORITY_LOW, NULL, 0);
    TEST_ASSERT(result == NO_ERROR, "Message creation without payload should succeed");
    TEST_ASSERT(msg.subject_id == 200, "Subject ID should be set correctly");
    TEST_ASSERT(msg.priority == CYPHAL_PRIORITY_LOW, "Priority should be set correctly");
    TEST_ASSERT(msg.payload_size == 0, "Payload size should be 0");
    TEST_ASSERT(msg.payload == NULL, "Payload should be NULL");
    
    uavcanMessageDestroy(&msg);
    
    // Test invalid parameters
    result = uavcanMessageCreate(NULL, 100, CYPHAL_PRIORITY_HIGH, test_payload, test_payload_size);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null message pointer should return error");
    
    result = uavcanMessageCreate(&msg, 100, 255, test_payload, test_payload_size);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Invalid priority should return error");
    
    result = uavcanMessageCreate(&msg, UAVCAN_SUBJECT_ID_MAX + 1, CYPHAL_PRIORITY_HIGH, test_payload, test_payload_size);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Invalid subject ID should return error");
    
    result = uavcanMessageCreate(&msg, 100, CYPHAL_PRIORITY_HIGH, test_payload, UAVCAN_MAX_PAYLOAD_SIZE + 1);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Oversized payload should return error");
    
    result = uavcanMessageCreate(&msg, 100, CYPHAL_PRIORITY_HIGH, NULL, 10);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null payload with non-zero size should return error");
}

/**
 * @brief Test message destruction
 */
static void test_uavcan_message_destroy(void)
{
    printf("\n=== Testing uavcanMessageDestroy ===\n");
    
    UavcanMessage msg;
    
    // Create message with payload
    error_t result = uavcanMessageCreate(&msg, 100, CYPHAL_PRIORITY_HIGH, test_payload, test_payload_size);
    TEST_ASSERT(result == NO_ERROR, "Message creation should succeed");
    
    // Destroy message
    result = uavcanMessageDestroy(&msg);
    TEST_ASSERT(result == NO_ERROR, "Message destruction should succeed");
    TEST_ASSERT(msg.payload == NULL, "Payload should be freed");
    TEST_ASSERT(msg.payload_size == 0, "Payload size should be reset");
    
    // Test destroying null pointer
    result = uavcanMessageDestroy(NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Destroying null pointer should return error");
}

/**
 * @brief Test validation functions
 */
static void test_uavcan_message_validation(void)
{
    printf("\n=== Testing validation functions ===\n");
    
    // Test priority validation
    TEST_ASSERT(uavcanMessageValidatePriority(0) == true, "Priority 0 should be valid");
    TEST_ASSERT(uavcanMessageValidatePriority(7) == true, "Priority 7 should be valid");
    TEST_ASSERT(uavcanMessageValidatePriority(8) == false, "Priority 8 should be invalid");
    TEST_ASSERT(uavcanMessageValidatePriority(255) == false, "Priority 255 should be invalid");
    
    // Test subject ID validation
    TEST_ASSERT(uavcanMessageValidateSubjectId(0) == true, "Subject ID 0 should be valid");
    TEST_ASSERT(uavcanMessageValidateSubjectId(UAVCAN_SUBJECT_ID_MAX) == true, "Max subject ID should be valid");
    TEST_ASSERT(uavcanMessageValidateSubjectId(UAVCAN_SUBJECT_ID_MAX + 1) == false, "Subject ID above max should be invalid");
    
    // Test payload size validation
    TEST_ASSERT(uavcanMessageValidatePayloadSize(0) == true, "Payload size 0 should be valid");
    TEST_ASSERT(uavcanMessageValidatePayloadSize(UAVCAN_MAX_PAYLOAD_SIZE) == true, "Max payload size should be valid");
    TEST_ASSERT(uavcanMessageValidatePayloadSize(UAVCAN_MAX_PAYLOAD_SIZE + 1) == false, "Payload size above max should be invalid");
    
    // Test complete message validation
    UavcanMessage msg;
    uavcanMessageCreate(&msg, 100, CYPHAL_PRIORITY_HIGH, test_payload, test_payload_size);
    TEST_ASSERT(uavcanMessageValidate(&msg) == true, "Valid message should pass validation");
    
    // Test invalid message validation
    msg.priority = 255;
    TEST_ASSERT(uavcanMessageValidate(&msg) == false, "Message with invalid priority should fail validation");
    
    msg.priority = CYPHAL_PRIORITY_HIGH;
    msg.subject_id = UAVCAN_SUBJECT_ID_MAX + 1;
    TEST_ASSERT(uavcanMessageValidate(&msg) == false, "Message with invalid subject ID should fail validation");
    
    TEST_ASSERT(uavcanMessageValidate(NULL) == false, "Null message should fail validation");
    
    uavcanMessageDestroy(&msg);
}

/**
 * @brief Test timestamp functionality
 */
static void test_uavcan_message_timestamp(void)
{
    printf("\n=== Testing timestamp functionality ===\n");
    
    UavcanMessage msg;
    uavcanMessageInit(&msg);
    
    // Test setting timestamp
    error_t result = uavcanMessageSetTimestamp(&msg);
    TEST_ASSERT(result == NO_ERROR, "Setting timestamp should succeed");
    TEST_ASSERT(msg.timestamp_usec > 0, "Timestamp should be greater than 0");
    
    uint64_t first_timestamp = msg.timestamp_usec;
    
    // Small delay and set timestamp again
    for (volatile int i = 0; i < 1000; i++);
    
    result = uavcanMessageSetTimestamp(&msg);
    TEST_ASSERT(result == NO_ERROR, "Setting timestamp again should succeed");
    TEST_ASSERT(msg.timestamp_usec >= first_timestamp, "Second timestamp should be >= first timestamp");
    
    // Test null pointer
    result = uavcanMessageSetTimestamp(NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Setting timestamp on null pointer should return error");
}

/**
 * @brief Test payload copy functionality
 */
static void test_uavcan_message_copy_payload(void)
{
    printf("\n=== Testing payload copy functionality ===\n");
    
    UavcanMessage msg;
    uavcanMessageInit(&msg);
    
    // Test copying payload
    error_t result = uavcanMessageCopyPayload(&msg, test_payload, test_payload_size);
    TEST_ASSERT(result == NO_ERROR, "Copying payload should succeed");
    TEST_ASSERT(msg.payload_size == test_payload_size, "Payload size should be set correctly");
    TEST_ASSERT(msg.payload != NULL, "Payload should be allocated");
    TEST_ASSERT(memcmp(msg.payload, test_payload, test_payload_size) == 0, "Payload data should be copied correctly");
    
    // Test copying different payload (should replace existing)
    const uint8_t new_payload[] = {0xAA, 0xBB, 0xCC};
    result = uavcanMessageCopyPayload(&msg, new_payload, sizeof(new_payload));
    TEST_ASSERT(result == NO_ERROR, "Copying new payload should succeed");
    TEST_ASSERT(msg.payload_size == sizeof(new_payload), "New payload size should be set correctly");
    TEST_ASSERT(memcmp(msg.payload, new_payload, sizeof(new_payload)) == 0, "New payload data should be copied correctly");
    
    // Test copying empty payload
    result = uavcanMessageCopyPayload(&msg, NULL, 0);
    TEST_ASSERT(result == NO_ERROR, "Copying empty payload should succeed");
    TEST_ASSERT(msg.payload_size == 0, "Payload size should be 0");
    TEST_ASSERT(msg.payload == NULL, "Payload should be NULL");
    
    // Test invalid parameters
    result = uavcanMessageCopyPayload(NULL, test_payload, test_payload_size);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null message pointer should return error");
    
    result = uavcanMessageCopyPayload(&msg, NULL, 10);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null payload with non-zero size should return error");
    
    result = uavcanMessageCopyPayload(&msg, test_payload, UAVCAN_MAX_PAYLOAD_SIZE + 1);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Oversized payload should return error");
    
    uavcanMessageDestroy(&msg);
}

/**
 * @brief Test message serialization and deserialization
 */
static void test_uavcan_message_serialization(void)
{
    printf("\n=== Testing serialization/deserialization ===\n");
    
    UavcanMessage original_msg, deserialized_msg;
    uint8_t buffer[256];
    size_t serialized_size;
    
    // Create test message
    error_t result = uavcanMessageCreate(&original_msg, 100, CYPHAL_PRIORITY_HIGH, 
                                        test_payload, test_payload_size);
    TEST_ASSERT(result == NO_ERROR, "Original message creation should succeed");
    
    original_msg.source_node_id = 42;
    original_msg.destination_node_id = 0;
    
    // Test serialization
    result = uavcanMessageSerialize(&original_msg, buffer, sizeof(buffer), &serialized_size);
    TEST_ASSERT(result == NO_ERROR, "Message serialization should succeed");
    TEST_ASSERT(serialized_size > 0, "Serialized size should be greater than 0");
    TEST_ASSERT(serialized_size <= sizeof(buffer), "Serialized size should fit in buffer");
    
    // Test deserialization
    result = uavcanMessageDeserialize(buffer, serialized_size, &deserialized_msg);
    TEST_ASSERT(result == NO_ERROR, "Message deserialization should succeed");
    
    // Verify deserialized message matches original
    TEST_ASSERT(deserialized_msg.subject_id == original_msg.subject_id, "Subject ID should match");
    TEST_ASSERT(deserialized_msg.priority == original_msg.priority, "Priority should match");
    TEST_ASSERT(deserialized_msg.source_node_id == original_msg.source_node_id, "Source node ID should match");
    TEST_ASSERT(deserialized_msg.destination_node_id == original_msg.destination_node_id, "Destination node ID should match");
    TEST_ASSERT(deserialized_msg.payload_size == original_msg.payload_size, "Payload size should match");
    
    if (deserialized_msg.payload_size > 0) {
        TEST_ASSERT(memcmp(deserialized_msg.payload, original_msg.payload, original_msg.payload_size) == 0, 
                   "Payload data should match");
    }
    
    // Test invalid parameters
    result = uavcanMessageSerialize(NULL, buffer, sizeof(buffer), &serialized_size);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null message should return error");
    
    result = uavcanMessageSerialize(&original_msg, NULL, sizeof(buffer), &serialized_size);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null buffer should return error");
    
    result = uavcanMessageDeserialize(NULL, serialized_size, &deserialized_msg);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null buffer should return error");
    
    result = uavcanMessageDeserialize(buffer, serialized_size, NULL);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null message should return error");
    
    // Clean up
    uavcanMessageDestroy(&original_msg);
    uavcanMessageDestroy(&deserialized_msg);
}

/**
 * @brief Test heartbeat message creation
 */
static void test_uavcan_message_heartbeat(void)
{
    printf("\n=== Testing heartbeat message creation ===\n");
    
    UavcanMessage msg;
    
    // Test successful heartbeat creation
    error_t result = uavcanMessageCreateHeartbeat(&msg, UAVCAN_NODE_HEALTH_NOMINAL, 
                                                 UAVCAN_NODE_MODE_OPERATIONAL, 12345);
    TEST_ASSERT(result == NO_ERROR, "Heartbeat creation should succeed");
    TEST_ASSERT(msg.subject_id == 7509, "Heartbeat should use correct subject ID");
    TEST_ASSERT(msg.priority == CYPHAL_PRIORITY_NOMINAL, "Heartbeat should use nominal priority");
    TEST_ASSERT(msg.payload_size == 8, "Heartbeat payload should be 8 bytes");
    TEST_ASSERT(msg.payload != NULL, "Heartbeat payload should be allocated");
    
    // Test invalid parameters
    result = uavcanMessageCreateHeartbeat(NULL, UAVCAN_NODE_HEALTH_NOMINAL, 
                                         UAVCAN_NODE_MODE_OPERATIONAL, 12345);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null message should return error");
    
    result = uavcanMessageCreateHeartbeat(&msg, 255, UAVCAN_NODE_MODE_OPERATIONAL, 12345);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Invalid health should return error");
    
    uavcanMessageDestroy(&msg);
}

/**
 * @brief Test node info message creation
 */
static void test_uavcan_message_node_info(void)
{
    printf("\n=== Testing node info message creation ===\n");
    
    UavcanMessage msg;
    const char* test_name = "TestNode";
    
    // Test successful node info creation
    error_t result = uavcanMessageCreateNodeInfo(&msg, test_name, 0x01020304, 0x05060708);
    TEST_ASSERT(result == NO_ERROR, "Node info creation should succeed");
    TEST_ASSERT(msg.subject_id == 430, "Node info should use correct subject ID");
    TEST_ASSERT(msg.priority == CYPHAL_PRIORITY_LOW, "Node info should use low priority");
    TEST_ASSERT(msg.payload_size > 0, "Node info payload should not be empty");
    TEST_ASSERT(msg.payload != NULL, "Node info payload should be allocated");
    
    // Test invalid parameters
    result = uavcanMessageCreateNodeInfo(NULL, test_name, 0x01020304, 0x05060708);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null message should return error");
    
    result = uavcanMessageCreateNodeInfo(&msg, NULL, 0x01020304, 0x05060708);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Null name should return error");
    
    // Test name too long
    char long_name[60];
    memset(long_name, 'A', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    result = uavcanMessageCreateNodeInfo(&msg, long_name, 0x01020304, 0x05060708);
    TEST_ASSERT(result == ERROR_INVALID_PARAMETER, "Too long name should return error");
    
    uavcanMessageDestroy(&msg);
}

/**
 * @brief Test serialized message validation
 */
static void test_uavcan_message_validate_serialized(void)
{
    printf("\n=== Testing serialized message validation ===\n");
    
    UavcanMessage msg;
    uint8_t buffer[256];
    size_t serialized_size;
    
    // Create and serialize a valid message
    error_t result = uavcanMessageCreate(&msg, 100, CYPHAL_PRIORITY_HIGH, test_payload, test_payload_size);
    TEST_ASSERT(result == NO_ERROR, "Message creation should succeed");
    
    result = uavcanMessageSerialize(&msg, buffer, sizeof(buffer), &serialized_size);
    TEST_ASSERT(result == NO_ERROR, "Message serialization should succeed");
    
    // Test validation of valid serialized message
    bool valid = uavcanMessageValidateSerialized(buffer, serialized_size);
    TEST_ASSERT(valid == true, "Valid serialized message should pass validation");
    
    // Test validation with invalid parameters
    valid = uavcanMessageValidateSerialized(NULL, serialized_size);
    TEST_ASSERT(valid == false, "Null buffer should fail validation");
    
    valid = uavcanMessageValidateSerialized(buffer, 5);
    TEST_ASSERT(valid == false, "Too small buffer should fail validation");
    
    // Test with corrupted subject ID
    uint8_t corrupted_buffer[256];
    memcpy(corrupted_buffer, buffer, serialized_size);
    corrupted_buffer[0] = 0xFF;
    corrupted_buffer[1] = 0xFF;
    corrupted_buffer[2] = 0xFF;
    corrupted_buffer[3] = 0xFF;
    valid = uavcanMessageValidateSerialized(corrupted_buffer, serialized_size);
    TEST_ASSERT(valid == false, "Corrupted subject ID should fail validation");
    
    uavcanMessageDestroy(&msg);
}

/**
 * @brief Run all tests
 */
void uavcanMessageHandlerRunTests(void)
{
    printf("Starting UAVCAN Message Handler Tests...\n");
    
    test_uavcan_message_init();
    test_uavcan_message_create();
    test_uavcan_message_destroy();
    test_uavcan_message_validation();
    test_uavcan_message_timestamp();
    test_uavcan_message_copy_payload();
    test_uavcan_message_serialization();
    test_uavcan_message_heartbeat();
    test_uavcan_message_node_info();
    test_uavcan_message_validate_serialized();
    
    printf("\n=== Test Results ===\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_run - tests_passed);
    printf("Success rate: %.1f%%\n", (float)tests_passed / tests_run * 100.0f);
    
    if (tests_passed == tests_run) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED!\n");
    }
}

// Main function for standalone testing
#ifdef UAVCAN_MESSAGE_HANDLER_TEST_STANDALONE
int main(void)
{
    uavcanMessageHandlerRunTests();
    return (tests_passed == tests_run) ? 0 : 1;
}
#endif
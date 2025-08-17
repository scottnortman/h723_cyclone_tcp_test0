#include "uavcan/uavcan_priority_queue.h"
#include "uavcan/uavcan_types.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Test framework macros
#define TEST_ASSERT(condition, message) \
    do { \
        if (!(condition)) { \
            printf("FAIL: %s - %s\n", __func__, message); \
            return false; \
        } \
    } while(0)

#define TEST_PASS() \
    do { \
        printf("PASS: %s\n", __func__); \
        return true; \
    } while(0)

// Helper function to create a test message
static UavcanMessage createTestMessage(uint8_t priority, uint32_t subject_id, const char* data)
{
    UavcanMessage msg = {0};
    msg.priority = priority;
    msg.subject_id = subject_id;
    msg.payload_size = strlen(data);
    msg.payload = (uint8_t*)data;
    msg.timestamp_usec = 1000000; // 1 second
    msg.source_node_id = 42;
    msg.destination_node_id = 0;
    msg.is_service_request = false;
    msg.is_anonymous = false;
    return msg;
}

// Test: Priority queue initialization
static bool test_priority_queue_init(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;

    // Test null parameter
    result = uavcanPriorityQueueInit(NULL);
    TEST_ASSERT(result == UAVCAN_ERROR_INVALID_PARAMETER, "Should reject null parameter");

    // Test successful initialization
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");
    TEST_ASSERT(pq.initialized == true, "Should be marked as initialized");
    TEST_ASSERT(pq.queue_mutex != NULL, "Should create mutex");

    // Verify all queues are created
    for (uint8_t i = 0; i < CYPHAL_PRIORITY_LEVELS; i++) {
        TEST_ASSERT(pq.priority_queues[i] != NULL, "All priority queues should be created");
        TEST_ASSERT(pq.queue_depths[i] > 0, "All queue depths should be set");
    }

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Priority queue deinitialization
static bool test_priority_queue_deinit(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;

    // Initialize first
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Test deinitialization
    result = uavcanPriorityQueueDeinit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should deinitialize successfully");
    TEST_ASSERT(pq.initialized == false, "Should be marked as not initialized");

    // Test null parameter
    result = uavcanPriorityQueueDeinit(NULL);
    TEST_ASSERT(result == UAVCAN_ERROR_INVALID_PARAMETER, "Should reject null parameter");

    TEST_PASS();
}

// Test: Priority validation
static bool test_priority_validation(void)
{
    // Test valid priorities
    for (uint8_t i = 0; i < CYPHAL_PRIORITY_LEVELS; i++) {
        TEST_ASSERT(uavcanPriorityQueueValidatePriority(i) == true, "Valid priorities should pass");
    }

    // Test invalid priorities
    TEST_ASSERT(uavcanPriorityQueueValidatePriority(CYPHAL_PRIORITY_LEVELS) == false, 
                "Priority 8 should be invalid");
    TEST_ASSERT(uavcanPriorityQueueValidatePriority(255) == false, 
                "Priority 255 should be invalid");

    TEST_PASS();
}

// Test: Message push operations
static bool test_message_push(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Test null parameters
    result = uavcanPriorityQueuePush(NULL, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_INVALID_PARAMETER, "Should reject null queue");

    result = uavcanPriorityQueuePush(&pq, NULL);
    TEST_ASSERT(result == UAVCAN_ERROR_INVALID_PARAMETER, "Should reject null message");

    // Test invalid priority
    msg = createTestMessage(255, 1000, "test");
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_INVALID_PARAMETER, "Should reject invalid priority");

    // Test successful push for each priority level
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        msg = createTestMessage(priority, 1000 + priority, "test data");
        result = uavcanPriorityQueuePush(&pq, &msg);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
    }

    // Verify queue has messages
    TEST_ASSERT(uavcanPriorityQueueHasMessages(&pq) == true, "Should have messages");
    TEST_ASSERT(uavcanPriorityQueueGetTotalCount(&pq) == CYPHAL_PRIORITY_LEVELS, 
                "Should have correct message count");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Message pop operations and priority ordering
static bool test_message_pop_priority_ordering(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg_in, msg_out;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Push messages in reverse priority order (lowest priority first)
    for (int8_t priority = CYPHAL_PRIORITY_LEVELS - 1; priority >= 0; priority--) {
        msg_in = createTestMessage((uint8_t)priority, 2000 + priority, "priority test");
        result = uavcanPriorityQueuePush(&pq, &msg_in);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
    }

    // Pop messages and verify they come out in priority order (0 = highest priority)
    for (uint8_t expected_priority = 0; expected_priority < CYPHAL_PRIORITY_LEVELS; expected_priority++) {
        result = uavcanPriorityQueuePop(&pq, &msg_out, 0);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should pop message successfully");
        TEST_ASSERT(msg_out.priority == expected_priority, "Should pop in priority order");
        TEST_ASSERT(msg_out.subject_id == 2000 + expected_priority, "Should have correct subject ID");
    }

    // Verify queue is empty
    TEST_ASSERT(uavcanPriorityQueueHasMessages(&pq) == false, "Should be empty");
    TEST_ASSERT(uavcanPriorityQueueGetTotalCount(&pq) == 0, "Should have zero messages");

    // Test pop from empty queue
    result = uavcanPriorityQueuePop(&pq, &msg_out, 0);
    TEST_ASSERT(result == UAVCAN_ERROR_TIMEOUT, "Should timeout on empty queue");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Queue overflow handling
static bool test_queue_overflow(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;
    UavcanPriorityQueueStats stats;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Fill up the highest priority queue beyond its capacity
    uint8_t test_priority = CYPHAL_PRIORITY_EXCEPTIONAL;
    uint32_t queue_depth = pq.queue_depths[test_priority];
    
    // Fill the queue to capacity
    for (uint32_t i = 0; i < queue_depth; i++) {
        msg = createTestMessage(test_priority, 3000 + i, "overflow test");
        result = uavcanPriorityQueuePush(&pq, &msg);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
    }

    // Try to push one more message (should overflow)
    msg = createTestMessage(test_priority, 4000, "overflow message");
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_QUEUE_FULL, "Should detect queue overflow");

    // Check overflow statistics
    result = uavcanPriorityQueueGetStats(&pq, test_priority, &stats);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should get stats successfully");
    TEST_ASSERT(stats.overflow_count > 0, "Should record overflow");
    TEST_ASSERT(pq.overflow_counts[test_priority] > 0, "Should record overflow in main structure");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Statistics tracking
static bool test_statistics_tracking(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;
    UavcanPriorityQueueStats stats;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    uint8_t test_priority = CYPHAL_PRIORITY_NOMINAL;
    uint32_t test_count = 5;

    // Push several messages
    for (uint32_t i = 0; i < test_count; i++) {
        msg = createTestMessage(test_priority, 5000 + i, "stats test");
        result = uavcanPriorityQueuePush(&pq, &msg);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
    }

    // Check statistics after pushing
    result = uavcanPriorityQueueGetStats(&pq, test_priority, &stats);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should get stats successfully");
    TEST_ASSERT(stats.messages_queued == test_count, "Should track queued messages");
    TEST_ASSERT(stats.current_depth == test_count, "Should track current depth");
    TEST_ASSERT(stats.max_depth_reached == test_count, "Should track max depth");

    // Pop some messages
    uint32_t pop_count = 3;
    for (uint32_t i = 0; i < pop_count; i++) {
        result = uavcanPriorityQueuePop(&pq, &msg, 0);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should pop message successfully");
    }

    // Check statistics after popping
    result = uavcanPriorityQueueGetStats(&pq, test_priority, &stats);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should get stats successfully");
    TEST_ASSERT(stats.messages_dequeued == pop_count, "Should track dequeued messages");
    TEST_ASSERT(stats.current_depth == (test_count - pop_count), "Should update current depth");
    TEST_ASSERT(stats.max_depth_reached == test_count, "Should maintain max depth");

    // Test statistics reset
    result = uavcanPriorityQueueResetStats(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should reset stats successfully");

    result = uavcanPriorityQueueGetStats(&pq, test_priority, &stats);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should get stats successfully");
    TEST_ASSERT(stats.messages_queued == 0, "Should reset queued count");
    TEST_ASSERT(stats.messages_dequeued == 0, "Should reset dequeued count");
    TEST_ASSERT(stats.overflow_count == 0, "Should reset overflow count");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Next priority detection
static bool test_next_priority_detection(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;
    uint8_t next_priority;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Test empty queue
    next_priority = uavcanPriorityQueueGetNextPriority(&pq);
    TEST_ASSERT(next_priority == CYPHAL_PRIORITY_LEVELS, "Should return no priority for empty queue");

    // Add message to low priority queue
    msg = createTestMessage(CYPHAL_PRIORITY_LOW, 6000, "low priority");
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");

    next_priority = uavcanPriorityQueueGetNextPriority(&pq);
    TEST_ASSERT(next_priority == CYPHAL_PRIORITY_LOW, "Should return low priority");

    // Add message to high priority queue
    msg = createTestMessage(CYPHAL_PRIORITY_FAST, 6001, "high priority");
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");

    next_priority = uavcanPriorityQueueGetNextPriority(&pq);
    TEST_ASSERT(next_priority == CYPHAL_PRIORITY_FAST, "Should return highest available priority");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Queue flush functionality
static bool test_queue_flush(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Add messages to multiple priority levels
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        msg = createTestMessage(priority, 7000 + priority, "flush test");
        result = uavcanPriorityQueuePush(&pq, &msg);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
    }

    // Verify messages are present
    TEST_ASSERT(uavcanPriorityQueueHasMessages(&pq) == true, "Should have messages");
    TEST_ASSERT(uavcanPriorityQueueGetTotalCount(&pq) == CYPHAL_PRIORITY_LEVELS, 
                "Should have correct message count");

    // Flush all queues
    result = uavcanPriorityQueueFlushAll(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should flush successfully");

    // Verify all messages are gone
    TEST_ASSERT(uavcanPriorityQueueHasMessages(&pq) == false, "Should have no messages");
    TEST_ASSERT(uavcanPriorityQueueGetTotalCount(&pq) == 0, "Should have zero messages");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Advanced priority-based queuing scenarios
static bool test_advanced_priority_queuing(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg_in, msg_out;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Test scenario: Multiple messages per priority level
    uint32_t messages_per_priority = 3;
    
    // Push multiple messages to each priority level
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        for (uint32_t i = 0; i < messages_per_priority; i++) {
            msg_in = createTestMessage(priority, 8000 + (priority * 100) + i, "multi-message test");
            result = uavcanPriorityQueuePush(&pq, &msg_in);
            TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
        }
    }

    // Verify total count
    uint32_t expected_total = CYPHAL_PRIORITY_LEVELS * messages_per_priority;
    TEST_ASSERT(uavcanPriorityQueueGetTotalCount(&pq) == expected_total, 
                "Should have correct total message count");

    // Pop all messages and verify they come out in strict priority order
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        for (uint32_t i = 0; i < messages_per_priority; i++) {
            result = uavcanPriorityQueuePop(&pq, &msg_out, 0);
            TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should pop message successfully");
            TEST_ASSERT(msg_out.priority == priority, "Should maintain strict priority order");
            
            // Verify subject ID matches expected pattern
            uint32_t expected_subject = 8000 + (priority * 100) + i;
            TEST_ASSERT(msg_out.subject_id == expected_subject, "Should have correct subject ID");
        }
    }

    // Verify queue is empty
    TEST_ASSERT(uavcanPriorityQueueGetTotalCount(&pq) == 0, "Should be empty after popping all");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Priority-specific queue operations
static bool test_priority_specific_operations(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Add messages to specific priority levels
    uint8_t test_priorities[] = {CYPHAL_PRIORITY_EXCEPTIONAL, CYPHAL_PRIORITY_NOMINAL, CYPHAL_PRIORITY_OPTIONAL};
    uint32_t num_test_priorities = sizeof(test_priorities) / sizeof(test_priorities[0]);

    for (uint32_t i = 0; i < num_test_priorities; i++) {
        msg = createTestMessage(test_priorities[i], 9000 + i, "priority specific test");
        result = uavcanPriorityQueuePush(&pq, &msg);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
    }

    // Test priority-specific count
    for (uint32_t i = 0; i < num_test_priorities; i++) {
        uint32_t count = uavcanPriorityQueueGetPriorityCount(&pq, test_priorities[i]);
        TEST_ASSERT(count == 1, "Should have one message in specific priority queue");
    }

    // Test flushing specific priority
    result = uavcanPriorityQueueFlushPriority(&pq, CYPHAL_PRIORITY_NOMINAL);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should flush specific priority successfully");

    // Verify only the specified priority was flushed
    TEST_ASSERT(uavcanPriorityQueueGetPriorityCount(&pq, CYPHAL_PRIORITY_EXCEPTIONAL) == 1, 
                "Should still have message in exceptional priority");
    TEST_ASSERT(uavcanPriorityQueueGetPriorityCount(&pq, CYPHAL_PRIORITY_NOMINAL) == 0, 
                "Should have no messages in nominal priority");
    TEST_ASSERT(uavcanPriorityQueueGetPriorityCount(&pq, CYPHAL_PRIORITY_OPTIONAL) == 1, 
                "Should still have message in optional priority");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Timeout-based push operations
static bool test_timeout_push_operations(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Test push with timeout (should succeed normally)
    msg = createTestMessage(CYPHAL_PRIORITY_FAST, 10000, "timeout test");
    result = uavcanPriorityQueuePushWithTimeout(&pq, &msg, 100);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push with timeout successfully");

    // Fill up a priority queue to test timeout on full queue
    uint8_t test_priority = CYPHAL_PRIORITY_FAST;
    uint32_t queue_depth = pq.queue_depths[test_priority];
    
    // Fill remaining capacity (we already have 1 message)
    for (uint32_t i = 1; i < queue_depth; i++) {
        msg = createTestMessage(test_priority, 10000 + i, "fill queue");
        result = uavcanPriorityQueuePushWithTimeout(&pq, &msg, 10);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
    }

    // Try to push one more with short timeout (should fail)
    msg = createTestMessage(test_priority, 11000, "overflow with timeout");
    result = uavcanPriorityQueuePushWithTimeout(&pq, &msg, 10);
    TEST_ASSERT(result == UAVCAN_ERROR_QUEUE_FULL, "Should fail due to queue full");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Comprehensive statistics for all priorities
static bool test_comprehensive_statistics(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;
    UavcanPriorityQueueStats all_stats[CYPHAL_PRIORITY_LEVELS];

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Add different numbers of messages to each priority level
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        uint32_t message_count = priority + 1; // 1 to 8 messages per priority
        
        for (uint32_t i = 0; i < message_count; i++) {
            msg = createTestMessage(priority, 12000 + (priority * 100) + i, "comprehensive stats");
            result = uavcanPriorityQueuePush(&pq, &msg);
            TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should push message successfully");
        }
    }

    // Get comprehensive statistics
    result = uavcanPriorityQueueGetAllStats(&pq, all_stats);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should get all stats successfully");

    // Verify statistics for each priority level
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        uint32_t expected_count = priority + 1;
        TEST_ASSERT(all_stats[priority].messages_queued == expected_count, 
                    "Should have correct queued count for each priority");
        TEST_ASSERT(all_stats[priority].current_depth == expected_count, 
                    "Should have correct current depth for each priority");
        TEST_ASSERT(all_stats[priority].max_depth_reached == expected_count, 
                    "Should have correct max depth for each priority");
    }

    // Pop some messages and verify statistics update
    uint32_t pop_count = 5;
    for (uint32_t i = 0; i < pop_count; i++) {
        result = uavcanPriorityQueuePop(&pq, &msg, 0);
        TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should pop message successfully");
    }

    // Get updated statistics
    result = uavcanPriorityQueueGetAllStats(&pq, all_stats);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should get updated stats successfully");

    // Verify that highest priority levels show dequeued messages
    uint32_t total_dequeued = 0;
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        total_dequeued += all_stats[priority].messages_dequeued;
    }
    TEST_ASSERT(total_dequeued == pop_count, "Should have correct total dequeued count");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Test: Edge cases and error conditions for priority queuing
static bool test_priority_queuing_edge_cases(void)
{
    UavcanPriorityQueue pq;
    UavcanError result;
    UavcanMessage msg;

    // Test operations on uninitialized queue
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_INIT_FAILED, "Should reject push on uninitialized queue");

    result = uavcanPriorityQueuePop(&pq, &msg, 0);
    TEST_ASSERT(result == UAVCAN_ERROR_INIT_FAILED, "Should reject pop on uninitialized queue");

    // Initialize queue
    result = uavcanPriorityQueueInit(&pq);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should initialize successfully");

    // Test invalid priority values
    msg = createTestMessage(CYPHAL_PRIORITY_LEVELS, 13000, "invalid priority");
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_INVALID_PARAMETER, "Should reject invalid priority");

    msg = createTestMessage(255, 13001, "invalid priority 255");
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_INVALID_PARAMETER, "Should reject invalid priority 255");

    // Test boundary priority values
    msg = createTestMessage(CYPHAL_PRIORITY_EXCEPTIONAL, 13002, "boundary test min");
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should accept minimum valid priority");

    msg = createTestMessage(CYPHAL_PRIORITY_OPTIONAL, 13003, "boundary test max");
    result = uavcanPriorityQueuePush(&pq, &msg);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should accept maximum valid priority");

    // Test pop order for boundary priorities
    result = uavcanPriorityQueuePop(&pq, &msg, 0);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should pop successfully");
    TEST_ASSERT(msg.priority == CYPHAL_PRIORITY_EXCEPTIONAL, "Should pop highest priority first");

    result = uavcanPriorityQueuePop(&pq, &msg, 0);
    TEST_ASSERT(result == UAVCAN_ERROR_NONE, "Should pop successfully");
    TEST_ASSERT(msg.priority == CYPHAL_PRIORITY_OPTIONAL, "Should pop lowest priority last");

    // Cleanup
    uavcanPriorityQueueDeinit(&pq);
    TEST_PASS();
}

// Main test runner
bool uavcanPriorityQueueRunTests(void)
{
    printf("Running UAVCAN Priority Queue Tests...\n");
    
    bool all_passed = true;
    
    // Basic functionality tests
    all_passed &= test_priority_queue_init();
    all_passed &= test_priority_queue_deinit();
    all_passed &= test_priority_validation();
    all_passed &= test_message_push();
    all_passed &= test_message_pop_priority_ordering();
    all_passed &= test_queue_overflow();
    all_passed &= test_statistics_tracking();
    all_passed &= test_next_priority_detection();
    all_passed &= test_queue_flush();
    
    // Advanced priority-based queuing tests
    all_passed &= test_advanced_priority_queuing();
    all_passed &= test_priority_specific_operations();
    all_passed &= test_timeout_push_operations();
    all_passed &= test_comprehensive_statistics();
    all_passed &= test_priority_queuing_edge_cases();
    
    if (all_passed) {
        printf("All UAVCAN Priority Queue tests PASSED!\n");
    } else {
        printf("Some UAVCAN Priority Queue tests FAILED!\n");
    }
    
    return all_passed;
}
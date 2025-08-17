#include "uavcan/uavcan_integration.h"
#include "uavcan/uavcan_priority_queue.h"
#include "uavcan/uavcan_message_handler.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Stress test configuration
#define STRESS_TEST_DURATION_MS     30000   // 30 seconds
#define STRESS_TEST_MESSAGE_RATE    100     // Messages per second
#define STRESS_TEST_BURST_SIZE      50      // Messages in burst
#define STRESS_TEST_BURST_INTERVAL  1000    // ms between bursts
#define STRESS_TEST_MAX_PAYLOAD     1024    // Maximum payload size

// Test task configuration
#define STRESS_PRODUCER_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)
#define STRESS_CONSUMER_TASK_PRIORITY   (tskIDLE_PRIORITY + 2)
#define STRESS_MONITOR_TASK_PRIORITY    (tskIDLE_PRIORITY + 1)

// Stress test statistics
typedef struct {
    uint32_t messages_generated;
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t messages_dropped;
    uint32_t queue_overflows;
    uint32_t send_errors;
    uint32_t receive_errors;
    uint32_t max_queue_depth;
    uint32_t total_latency_us;
    uint32_t max_latency_us;
    uint32_t min_latency_us;
    bool test_running;
    bool test_completed;
} UavcanStressTestStats;

// Global test context
static UavcanStressTestStats g_stress_stats = {0};
static SemaphoreHandle_t g_stats_mutex = NULL;
static TaskHandle_t g_producer_task = NULL;
static TaskHandle_t g_consumer_task = NULL;
static TaskHandle_t g_monitor_task = NULL;
static UavcanIntegrationContext* g_test_context = NULL;

/**
 * @brief Update stress test statistics (thread-safe)
 */
static void updateStressStats(void (*update_func)(UavcanStressTestStats*)) {
    if (g_stats_mutex && xSemaphoreTake(g_stats_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        update_func(&g_stress_stats);
        xSemaphoreGive(g_stats_mutex);
    }
}

/**
 * @brief Message producer task - generates high load of messages
 */
static void stressTestProducerTask(void* pvParameters) {
    printf("Stress test producer task started\n");
    
    uint32_t message_counter = 0;
    uint32_t start_time = xTaskGetTickCount();
    uint32_t next_burst_time = start_time;
    
    while (g_stress_stats.test_running) {
        uint32_t current_time = xTaskGetTickCount();
        
        // Check if test duration exceeded
        if ((current_time - start_time) * portTICK_PERIOD_MS >= STRESS_TEST_DURATION_MS) {
            break;
        }
        
        // Generate burst of messages
        if (current_time >= next_burst_time) {
            for (int i = 0; i < STRESS_TEST_BURST_SIZE && g_stress_stats.test_running; i++) {
                // Create test message with random priority and payload size
                uint8_t priority = rand() % CYPHAL_PRIORITY_LEVELS;
                uint16_t payload_size = (rand() % STRESS_TEST_MAX_PAYLOAD) + 1;
                uint32_t subject_id = 1000 + (rand() % 1000);
                
                // Allocate payload
                uint8_t* payload = malloc(payload_size);
                if (!payload) {
                    updateStressStats([](UavcanStressTestStats* stats) {
                        stats->send_errors++;
                    });
                    continue;
                }
                
                // Fill payload with test data
                for (int j = 0; j < payload_size; j++) {
                    payload[j] = (uint8_t)(message_counter + j);
                }
                
                // Create message
                UavcanMessage msg = {
                    .subject_id = subject_id,
                    .priority = priority,
                    .payload_size = payload_size,
                    .payload = payload,
                    .source_node_id = 42,
                    .timestamp_usec = xTaskGetTickCount() * 1000ULL * portTICK_PERIOD_MS
                };
                
                // Try to send message
                UavcanError result = uavcanMessageSend(&msg);
                
                updateStressStats([=](UavcanStressTestStats* stats) {
                    stats->messages_generated++;
                    if (result == UAVCAN_ERROR_NONE) {
                        stats->messages_sent++;
                    } else if (result == UAVCAN_ERROR_QUEUE_FULL) {
                        stats->queue_overflows++;
                        stats->messages_dropped++;
                    } else {
                        stats->send_errors++;
                    }
                });
                
                free(payload);
                message_counter++;
                
                // Small delay between messages in burst
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            
            next_burst_time = current_time + pdMS_TO_TICKS(STRESS_TEST_BURST_INTERVAL);
        }
        
        // Delay until next burst
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    
    printf("Stress test producer task completed\n");
    vTaskDelete(NULL);
}

/**
 * @brief Message consumer task - processes received messages
 */
static void stressTestConsumerTask(void* pvParameters) {
    printf("Stress test consumer task started\n");
    
    while (g_stress_stats.test_running) {
        UavcanMessage msg;
        UavcanError result = uavcanMessageReceive(&msg, 100);  // 100ms timeout
        
        if (result == UAVCAN_ERROR_NONE) {
            // Calculate latency
            uint64_t current_time_us = xTaskGetTickCount() * 1000ULL * portTICK_PERIOD_MS;
            uint32_t latency_us = (uint32_t)(current_time_us - msg.timestamp_usec);
            
            updateStressStats([=](UavcanStressTestStats* stats) {
                stats->messages_received++;
                stats->total_latency_us += latency_us;
                
                if (latency_us > stats->max_latency_us) {
                    stats->max_latency_us = latency_us;
                }
                
                if (stats->min_latency_us == 0 || latency_us < stats->min_latency_us) {
                    stats->min_latency_us = latency_us;
                }
            });
            
            // Validate message content
            if (msg.payload && msg.payload_size > 0) {
                // Simple validation - check if payload has expected pattern
                bool valid = true;
                for (size_t i = 1; i < msg.payload_size && valid; i++) {
                    if (msg.payload[i] != (uint8_t)(msg.payload[0] + i)) {
                        valid = false;
                    }
                }
                
                if (!valid) {
                    updateStressStats([](UavcanStressTestStats* stats) {
                        stats->receive_errors++;
                    });
                }
            }
        } else if (result != UAVCAN_ERROR_TIMEOUT) {
            updateStressStats([](UavcanStressTestStats* stats) {
                stats->receive_errors++;
            });
        }
        
        // Small processing delay
        vTaskDelay(pdMS_TO_TICKS(5));
    }
    
    printf("Stress test consumer task completed\n");
    vTaskDelete(NULL);
}

/**
 * @brief Monitor task - tracks system health during stress test
 */
static void stressTestMonitorTask(void* pvParameters) {
    printf("Stress test monitor task started\n");
    
    uint32_t last_report_time = xTaskGetTickCount();
    
    while (g_stress_stats.test_running) {
        uint32_t current_time = xTaskGetTickCount();
        
        // Report statistics every 5 seconds
        if ((current_time - last_report_time) * portTICK_PERIOD_MS >= 5000) {
            if (g_stats_mutex && xSemaphoreTake(g_stats_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                printf("Stress Test Progress:\n");
                printf("  Messages Generated: %lu\n", g_stress_stats.messages_generated);
                printf("  Messages Sent: %lu\n", g_stress_stats.messages_sent);
                printf("  Messages Received: %lu\n", g_stress_stats.messages_received);
                printf("  Messages Dropped: %lu\n", g_stress_stats.messages_dropped);
                printf("  Queue Overflows: %lu\n", g_stress_stats.queue_overflows);
                printf("  Send Errors: %lu\n", g_stress_stats.send_errors);
                printf("  Receive Errors: %lu\n", g_stress_stats.receive_errors);
                
                if (g_stress_stats.messages_received > 0) {
                    uint32_t avg_latency = g_stress_stats.total_latency_us / g_stress_stats.messages_received;
                    printf("  Avg Latency: %lu us\n", avg_latency);
                    printf("  Max Latency: %lu us\n", g_stress_stats.max_latency_us);
                    printf("  Min Latency: %lu us\n", g_stress_stats.min_latency_us);
                }
                
                xSemaphoreGive(g_stats_mutex);
            }
            
            last_report_time = current_time;
        }
        
        // Check system health
        if (g_test_context) {
            bool is_ready = uavcanIntegrationIsReady(g_test_context);
            if (!is_ready) {
                printf("WARNING: UAVCAN system not ready during stress test\n");
            }
            
            // Update system
            uavcanIntegrationUpdate(g_test_context);
        }
        
        // Check free heap
        size_t free_heap = xPortGetFreeHeapSize();
        static size_t min_free_heap = SIZE_MAX;
        if (free_heap < min_free_heap) {
            min_free_heap = free_heap;
            printf("Free heap: %zu bytes (minimum so far)\n", min_free_heap);
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second monitoring interval
    }
    
    printf("Stress test monitor task completed\n");
    vTaskDelete(NULL);
}

/**
 * @brief Run UAVCAN stress test with high message loads
 */
bool uavcanRunStressTest(UavcanIntegrationContext* ctx) {
    if (!ctx) {
        printf("ERROR: Invalid context for stress test\n");
        return false;
    }
    
    printf("UAVCAN Stress Test\n");
    printf("==================\n");
    printf("Duration: %d seconds\n", STRESS_TEST_DURATION_MS / 1000);
    printf("Message rate: %d messages/second (in bursts)\n", STRESS_TEST_MESSAGE_RATE);
    printf("Burst size: %d messages\n", STRESS_TEST_BURST_SIZE);
    printf("Burst interval: %d ms\n", STRESS_TEST_BURST_INTERVAL);
    printf("\n");
    
    // Initialize test
    memset(&g_stress_stats, 0, sizeof(g_stress_stats));
    g_stress_stats.test_running = true;
    g_stress_stats.min_latency_us = UINT32_MAX;
    g_test_context = ctx;
    
    // Create mutex for statistics
    g_stats_mutex = xSemaphoreCreateMutex();
    if (!g_stats_mutex) {
        printf("ERROR: Failed to create statistics mutex\n");
        return false;
    }
    
    // Create test tasks
    BaseType_t result;
    
    result = xTaskCreate(stressTestProducerTask, "StressProducer", 
                        configMINIMAL_STACK_SIZE * 2, NULL, 
                        STRESS_PRODUCER_TASK_PRIORITY, &g_producer_task);
    if (result != pdPASS) {
        printf("ERROR: Failed to create producer task\n");
        vSemaphoreDelete(g_stats_mutex);
        return false;
    }
    
    result = xTaskCreate(stressTestConsumerTask, "StressConsumer", 
                        configMINIMAL_STACK_SIZE * 2, NULL, 
                        STRESS_CONSUMER_TASK_PRIORITY, &g_consumer_task);
    if (result != pdPASS) {
        printf("ERROR: Failed to create consumer task\n");
        vTaskDelete(g_producer_task);
        vSemaphoreDelete(g_stats_mutex);
        return false;
    }
    
    result = xTaskCreate(stressTestMonitorTask, "StressMonitor", 
                        configMINIMAL_STACK_SIZE * 2, NULL, 
                        STRESS_MONITOR_TASK_PRIORITY, &g_monitor_task);
    if (result != pdPASS) {
        printf("ERROR: Failed to create monitor task\n");
        vTaskDelete(g_producer_task);
        vTaskDelete(g_consumer_task);
        vSemaphoreDelete(g_stats_mutex);
        return false;
    }
    
    printf("Stress test tasks created, starting test...\n");
    
    // Wait for test completion
    uint32_t start_time = xTaskGetTickCount();
    while (g_stress_stats.test_running) {
        uint32_t elapsed_ms = (xTaskGetTickCount() - start_time) * portTICK_PERIOD_MS;
        if (elapsed_ms >= STRESS_TEST_DURATION_MS) {
            g_stress_stats.test_running = false;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Wait for tasks to complete
    printf("Stopping stress test tasks...\n");
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Print final results
    printf("\nStress Test Results:\n");
    printf("====================\n");
    printf("Messages Generated: %lu\n", g_stress_stats.messages_generated);
    printf("Messages Sent: %lu\n", g_stress_stats.messages_sent);
    printf("Messages Received: %lu\n", g_stress_stats.messages_received);
    printf("Messages Dropped: %lu\n", g_stress_stats.messages_dropped);
    printf("Queue Overflows: %lu\n", g_stress_stats.queue_overflows);
    printf("Send Errors: %lu\n", g_stress_stats.send_errors);
    printf("Receive Errors: %lu\n", g_stress_stats.receive_errors);
    
    if (g_stress_stats.messages_received > 0) {
        uint32_t avg_latency = g_stress_stats.total_latency_us / g_stress_stats.messages_received;
        printf("Average Latency: %lu us\n", avg_latency);
        printf("Maximum Latency: %lu us\n", g_stress_stats.max_latency_us);
        printf("Minimum Latency: %lu us\n", g_stress_stats.min_latency_us);
        
        float success_rate = (float)g_stress_stats.messages_received / g_stress_stats.messages_sent * 100.0f;
        printf("Message Success Rate: %.2f%%\n", success_rate);
    }
    
    // Evaluate test results
    bool test_passed = true;
    
    if (g_stress_stats.messages_sent == 0) {
        printf("FAIL: No messages were sent\n");
        test_passed = false;
    }
    
    if (g_stress_stats.messages_received == 0) {
        printf("FAIL: No messages were received\n");
        test_passed = false;
    }
    
    float message_loss_rate = (float)g_stress_stats.messages_dropped / g_stress_stats.messages_generated * 100.0f;
    if (message_loss_rate > 10.0f) {  // Allow up to 10% message loss under extreme stress
        printf("FAIL: High message loss rate: %.2f%%\n", message_loss_rate);
        test_passed = false;
    }
    
    if (g_stress_stats.receive_errors > g_stress_stats.messages_received / 10) {  // Allow up to 10% receive errors
        printf("FAIL: High receive error rate\n");
        test_passed = false;
    }
    
    // Clean up
    vSemaphoreDelete(g_stats_mutex);
    g_stats_mutex = NULL;
    g_test_context = NULL;
    
    printf("\n");
    if (test_passed) {
        printf("STRESS TEST PASSED! ✓\n");
        printf("System remained stable under high message load.\n");
    } else {
        printf("STRESS TEST FAILED! ✗\n");
        printf("System showed instability under high message load.\n");
    }
    
    return test_passed;
}
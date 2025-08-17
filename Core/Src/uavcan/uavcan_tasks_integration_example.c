/**
 * @file uavcan_tasks_integration_example.c
 * @brief Example showing how to integrate UAVCAN task architecture into main application
 */

#include "uavcan/uavcan_tasks.h"
#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_priority_queue.h"
#include "uavcan/uavcan_udp_transport.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

// Global UAVCAN components
static UavcanTaskContext g_uavcan_task_ctx;
static UavcanNodeContext g_uavcan_node_ctx;
static UavcanPriorityQueue g_uavcan_priority_queue;
static UavcanUdpTransport g_uavcan_udp_transport;

// Configuration
#define EXAMPLE_NODE_ID 42
#define EXAMPLE_HEARTBEAT_INTERVAL_MS 1000

/**
 * @brief Initialize UAVCAN subsystem
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanSubsystemInit(void) {
    error_t result;

    printf("Initializing UAVCAN subsystem...\n");

    // Initialize node context
    result = uavcanNodeInit(&g_uavcan_node_ctx, EXAMPLE_NODE_ID);
    if (UAVCAN_FAILED(result)) {
        printf("Failed to initialize UAVCAN node: %d\n", result);
        return result;
    }

    // Initialize priority queue
    result = uavcanPriorityQueueInit(&g_uavcan_priority_queue);
    if (UAVCAN_FAILED(result)) {
        printf("Failed to initialize priority queue: %d\n", result);
        return result;
    }

    // Initialize UDP transport
    result = uavcanUdpTransportInit(&g_uavcan_udp_transport);
    if (UAVCAN_FAILED(result)) {
        printf("Failed to initialize UDP transport: %d\n", result);
        return result;
    }

    // Initialize task architecture
    result = uavcanTasksInit(&g_uavcan_task_ctx, 
                            &g_uavcan_node_ctx,
                            &g_uavcan_priority_queue,
                            &g_uavcan_udp_transport);
    if (UAVCAN_FAILED(result)) {
        printf("Failed to initialize UAVCAN tasks: %d\n", result);
        return result;
    }

    printf("UAVCAN subsystem initialized successfully\n");
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Start UAVCAN subsystem
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanSubsystemStart(void) {
    error_t result;

    printf("Starting UAVCAN subsystem...\n");

    // Start UAVCAN tasks
    result = uavcanTasksStart(&g_uavcan_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("Failed to start UAVCAN tasks: %d\n", result);
        return result;
    }

    // Wait for tasks to start
    TickType_t start_time = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(5000); // 5 second timeout

    while ((xTaskGetTickCount() - start_time) < timeout) {
        if (uavcanTasksAreRunning(&g_uavcan_task_ctx)) {
            printf("UAVCAN subsystem started successfully\n");
            return UAVCAN_ERROR_NONE;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    printf("UAVCAN subsystem failed to start within timeout\n");
    return UAVCAN_ERROR_TIMEOUT;
}

/**
 * @brief Stop UAVCAN subsystem
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanSubsystemStop(void) {
    printf("Stopping UAVCAN subsystem...\n");

    error_t result = uavcanTasksStop(&g_uavcan_task_ctx);
    if (UAVCAN_FAILED(result)) {
        printf("Failed to stop UAVCAN tasks: %d\n", result);
        return result;
    }

    printf("UAVCAN subsystem stopped\n");
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get UAVCAN subsystem status
 * @param buffer Buffer to store status string
 * @param buffer_size Size of buffer
 * @return Number of characters written
 */
size_t uavcanSubsystemGetStatus(char* buffer, size_t buffer_size) {
    if (buffer == NULL || buffer_size == 0) {
        return 0;
    }

    // Get node status
    char node_status[256];
    uavcanNodeGetStatusString(&g_uavcan_node_ctx, node_status, sizeof(node_status));

    // Get task status
    char task_status[256];
    uavcanTasksGetStatusString(&g_uavcan_task_ctx, task_status, sizeof(task_status));

    // Get task statistics
    uint32_t node_cycles, tx_cycles, rx_cycles;
    uavcanTasksGetStatistics(&g_uavcan_task_ctx, &node_cycles, &tx_cycles, &rx_cycles);

    return snprintf(buffer, buffer_size,
                   "=== UAVCAN Subsystem Status ===\n"
                   "%s\n"
                   "%s\n"
                   "Task Cycles: Node=%lu, TX=%lu, RX=%lu\n"
                   "===============================",
                   node_status,
                   task_status,
                   (unsigned long)node_cycles,
                   (unsigned long)tx_cycles,
                   (unsigned long)rx_cycles);
}

/**
 * @brief Send a test message via UAVCAN
 * @param subject_id Subject ID for the message
 * @param priority Message priority (0-7)
 * @param data Pointer to message data
 * @param data_size Size of message data
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanSubsystemSendTestMessage(uint32_t subject_id, 
                                      uint8_t priority,
                                      const void* data,
                                      size_t data_size) {
    if (!uavcanTasksAreRunning(&g_uavcan_task_ctx)) {
        printf("UAVCAN tasks not running, cannot send message\n");
        return UAVCAN_ERROR_INIT_FAILED;
    }

    UavcanMessage message;
    error_t result = uavcanMessageCreate(&message, subject_id, priority, data, data_size);
    if (UAVCAN_FAILED(result)) {
        printf("Failed to create test message: %d\n", result);
        return result;
    }

    result = uavcanPriorityQueuePush(&g_uavcan_priority_queue, &message);
    if (UAVCAN_FAILED(result)) {
        printf("Failed to queue test message: %d\n", result);
        return result;
    }

    printf("Test message queued successfully (Subject ID: %lu, Priority: %d)\n",
           (unsigned long)subject_id, priority);
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Example task that demonstrates UAVCAN integration
 */
static void uavcanExampleTask(void* pvParameters) {
    (void)pvParameters;

    printf("UAVCAN Example Task started\n");

    // Initialize UAVCAN subsystem
    error_t result = uavcanSubsystemInit();
    if (UAVCAN_FAILED(result)) {
        printf("Failed to initialize UAVCAN subsystem, task exiting\n");
        vTaskDelete(NULL);
        return;
    }

    // Start UAVCAN subsystem
    result = uavcanSubsystemStart();
    if (UAVCAN_FAILED(result)) {
        printf("Failed to start UAVCAN subsystem, task exiting\n");
        vTaskDelete(NULL);
        return;
    }

    // Main loop
    TickType_t last_status_time = xTaskGetTickCount();
    TickType_t last_test_message_time = xTaskGetTickCount();
    const TickType_t status_interval = pdMS_TO_TICKS(10000); // 10 seconds
    const TickType_t test_message_interval = pdMS_TO_TICKS(5000); // 5 seconds

    while (1) {
        TickType_t current_time = xTaskGetTickCount();

        // Print status periodically
        if ((current_time - last_status_time) >= status_interval) {
            char status_buffer[512];
            size_t status_len = uavcanSubsystemGetStatus(status_buffer, sizeof(status_buffer));
            if (status_len > 0) {
                printf("\n%s\n\n", status_buffer);
            }
            last_status_time = current_time;
        }

        // Send test message periodically
        if ((current_time - last_test_message_time) >= test_message_interval) {
            const char* test_data = "Hello UAVCAN!";
            uavcanSubsystemSendTestMessage(1000, // Test subject ID
                                         CYPHAL_PRIORITY_NOMINAL,
                                         test_data,
                                         strlen(test_data));
            last_test_message_time = current_time;
        }

        // Task delay
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/**
 * @brief Start the UAVCAN example task
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanExampleStart(void) {
    BaseType_t result = xTaskCreate(
        uavcanExampleTask,
        "UavcanExample",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        tskIDLE_PRIORITY + 2,
        NULL
    );

    if (result != pdPASS) {
        printf("Failed to create UAVCAN example task\n");
        return UAVCAN_ERROR_INIT_FAILED;
    }

    printf("UAVCAN example task created successfully\n");
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Integration function to be called from main application
 * This shows how to integrate UAVCAN into the existing application
 */
void integrateUavcanIntoMainApplication(void) {
    printf("Integrating UAVCAN into main application...\n");

    // This function would be called from main() after other initialization
    // but before starting the FreeRTOS scheduler

    error_t result = uavcanExampleStart();
    if (UAVCAN_FAILED(result)) {
        printf("Failed to start UAVCAN example: %d\n", result);
        return;
    }

    printf("UAVCAN integration completed\n");
}

/**
 * @brief Console command handler for UAVCAN status
 * This shows how to integrate with the existing command console
 */
void uavcanConsoleStatusCommand(void) {
    char status_buffer[512];
    size_t status_len = uavcanSubsystemGetStatus(status_buffer, sizeof(status_buffer));
    
    if (status_len > 0) {
        printf("%s\n", status_buffer);
    } else {
        printf("Failed to get UAVCAN status\n");
    }
}

/**
 * @brief Console command handler for sending UAVCAN test message
 */
void uavcanConsoleSendTestCommand(uint32_t subject_id, uint8_t priority) {
    const char* test_data = "Console test message";
    error_t result = uavcanSubsystemSendTestMessage(subject_id, priority, test_data, strlen(test_data));
    
    if (UAVCAN_SUCCEEDED(result)) {
        printf("Test message sent successfully\n");
    } else {
        printf("Failed to send test message: %d\n", result);
    }
}
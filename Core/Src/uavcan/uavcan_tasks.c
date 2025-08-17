#include "uavcan/uavcan_tasks.h"
#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_priority_queue.h"
#include "uavcan/uavcan_udp_transport.h"
#include "uavcan/uavcan_message_handler.h"
#include <string.h>
#include <stdio.h>

// Task function prototypes
static void uavcanNodeTask(void* pvParameters);
static void uavcanTxTask(void* pvParameters);
static void uavcanRxTask(void* pvParameters);

// Helper function prototypes
static error_t processNodeCommand(UavcanTaskContext* ctx, const UavcanTaskCommandMessage* cmd);
static void updateTaskState(UavcanTaskContext* ctx, UavcanTaskState* state, UavcanTaskState new_state);
static const char* taskStateToString(UavcanTaskState state);

/**
 * @brief Initialize UAVCAN task architecture
 */
error_t uavcanTasksInit(UavcanTaskContext* ctx, 
                       UavcanNodeContext* node_ctx,
                       void* priority_queue,
                       void* udp_transport) {
    if (ctx == NULL || node_ctx == NULL || priority_queue == NULL || udp_transport == NULL) {
        UAVCAN_ERROR_PRINT("Invalid parameters for task initialization");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Reset context to ensure clean state
    uavcanTasksReset(ctx);

    // Store references
    ctx->node_context = node_ctx;
    ctx->priority_queue = priority_queue;
    ctx->udp_transport = udp_transport;

    // Create synchronization primitives
    ctx->state_mutex = xSemaphoreCreateMutex();
    if (ctx->state_mutex == NULL) {
        UAVCAN_ERROR_PRINT("Failed to create state mutex");
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    // Create command queue for node task
    ctx->node_command_queue = xQueueCreate(8, sizeof(UavcanTaskCommandMessage));
    if (ctx->node_command_queue == NULL) {
        UAVCAN_ERROR_PRINT("Failed to create node command queue");
        vSemaphoreDelete(ctx->state_mutex);
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    // Create message queues for inter-task communication
    ctx->tx_message_queue = xQueueCreate(16, sizeof(UavcanMessage*));
    if (ctx->tx_message_queue == NULL) {
        UAVCAN_ERROR_PRINT("Failed to create TX message queue");
        vQueueDelete(ctx->node_command_queue);
        vSemaphoreDelete(ctx->state_mutex);
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    ctx->rx_message_queue = xQueueCreate(16, sizeof(UavcanMessage*));
    if (ctx->rx_message_queue == NULL) {
        UAVCAN_ERROR_PRINT("Failed to create RX message queue");
        vQueueDelete(ctx->tx_message_queue);
        vQueueDelete(ctx->node_command_queue);
        vSemaphoreDelete(ctx->state_mutex);
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    // Initialize task states
    ctx->node_task_state = UAVCAN_TASK_STATE_STOPPED;
    ctx->tx_task_state = UAVCAN_TASK_STATE_STOPPED;
    ctx->rx_task_state = UAVCAN_TASK_STATE_STOPPED;

    UAVCAN_INFO_PRINT("UAVCAN task architecture initialized");
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Start all UAVCAN tasks
 */
error_t uavcanTasksStart(UavcanTaskContext* ctx) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Task context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (ctx->tasks_started) {
        UAVCAN_WARN_PRINT("UAVCAN tasks already started");
        return UAVCAN_ERROR_NONE;
    }

    BaseType_t result;

    // Create UAVCAN Node Task
    result = xTaskCreate(
        uavcanNodeTask,
        "UavcanNode",
        UAVCAN_NODE_TASK_STACK_SIZE,
        ctx,
        UAVCAN_NODE_TASK_PRIORITY,
        &ctx->node_task_handle
    );
    if (result != pdPASS) {
        UAVCAN_ERROR_PRINT("Failed to create UAVCAN node task");
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Create UAVCAN TX Task
    result = xTaskCreate(
        uavcanTxTask,
        "UavcanTx",
        UAVCAN_TX_TASK_STACK_SIZE,
        ctx,
        UAVCAN_TX_TASK_PRIORITY,
        &ctx->tx_task_handle
    );
    if (result != pdPASS) {
        UAVCAN_ERROR_PRINT("Failed to create UAVCAN TX task");
        vTaskDelete(ctx->node_task_handle);
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Create UAVCAN RX Task
    result = xTaskCreate(
        uavcanRxTask,
        "UavcanRx",
        UAVCAN_RX_TASK_STACK_SIZE,
        ctx,
        UAVCAN_RX_TASK_PRIORITY,
        &ctx->rx_task_handle
    );
    if (result != pdPASS) {
        UAVCAN_ERROR_PRINT("Failed to create UAVCAN RX task");
        vTaskDelete(ctx->tx_task_handle);
        vTaskDelete(ctx->node_task_handle);
        return UAVCAN_ERROR_INIT_FAILED;
    }

    ctx->tasks_started = true;
    UAVCAN_INFO_PRINT("UAVCAN tasks started successfully");
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Stop all UAVCAN tasks
 */
error_t uavcanTasksStop(UavcanTaskContext* ctx) {
    if (ctx == NULL) {
        UAVCAN_ERROR_PRINT("Task context is NULL");
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!ctx->tasks_started) {
        UAVCAN_WARN_PRINT("UAVCAN tasks not started");
        return UAVCAN_ERROR_NONE;
    }

    // Send stop command to node task
    UavcanTaskCommandMessage stop_cmd = {
        .command = UAVCAN_TASK_CMD_STOP,
        .data = NULL,
        .data_size = 0
    };

    if (xQueueSend(ctx->node_command_queue, &stop_cmd, pdMS_TO_TICKS(1000)) != pdTRUE) {
        UAVCAN_WARN_PRINT("Failed to send stop command to node task");
    }

    // Wait for tasks to stop gracefully (with timeout)
    TickType_t start_time = xTaskGetTickCount();
    const TickType_t timeout = pdMS_TO_TICKS(5000); // 5 second timeout

    while ((xTaskGetTickCount() - start_time) < timeout) {
        if (ctx->node_task_state == UAVCAN_TASK_STATE_STOPPED &&
            ctx->tx_task_state == UAVCAN_TASK_STATE_STOPPED &&
            ctx->rx_task_state == UAVCAN_TASK_STATE_STOPPED) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    // Force delete tasks if they didn't stop gracefully
    if (ctx->node_task_handle != NULL) {
        vTaskDelete(ctx->node_task_handle);
        ctx->node_task_handle = NULL;
    }
    if (ctx->tx_task_handle != NULL) {
        vTaskDelete(ctx->tx_task_handle);
        ctx->tx_task_handle = NULL;
    }
    if (ctx->rx_task_handle != NULL) {
        vTaskDelete(ctx->rx_task_handle);
        ctx->rx_task_handle = NULL;
    }

    ctx->tasks_started = false;
    UAVCAN_INFO_PRINT("UAVCAN tasks stopped");
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Send command to node task
 */
error_t uavcanTasksSendCommand(UavcanTaskContext* ctx,
                              UavcanTaskCommand command,
                              void* data,
                              size_t data_size) {
    if (ctx == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    UavcanTaskCommandMessage cmd = {
        .command = command,
        .data = data,
        .data_size = data_size
    };

    if (xQueueSend(ctx->node_command_queue, &cmd, pdMS_TO_TICKS(1000)) != pdTRUE) {
        UAVCAN_ERROR_PRINT("Failed to send command %d to node task", command);
        return UAVCAN_ERROR_QUEUE_FULL;
    }

    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get task states
 */
error_t uavcanTasksGetStates(const UavcanTaskContext* ctx,
                            UavcanTaskState* node_state,
                            UavcanTaskState* tx_state,
                            UavcanTaskState* rx_state) {
    if (ctx == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(ctx->state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (node_state) *node_state = ctx->node_task_state;
        if (tx_state) *tx_state = ctx->tx_task_state;
        if (rx_state) *rx_state = ctx->rx_task_state;
        xSemaphoreGive(ctx->state_mutex);
        return UAVCAN_ERROR_NONE;
    }

    return UAVCAN_ERROR_TIMEOUT;
}

/**
 * @brief Get task statistics
 */
error_t uavcanTasksGetStatistics(const UavcanTaskContext* ctx,
                                uint32_t* node_cycles,
                                uint32_t* tx_cycles,
                                uint32_t* rx_cycles) {
    if (ctx == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (node_cycles) *node_cycles = ctx->node_task_cycles;
    if (tx_cycles) *tx_cycles = ctx->tx_task_cycles;
    if (rx_cycles) *rx_cycles = ctx->rx_task_cycles;

    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Check if all tasks are running
 */
bool uavcanTasksAreRunning(const UavcanTaskContext* ctx) {
    if (ctx == NULL) {
        return false;
    }

    return (ctx->tasks_started &&
            ctx->node_task_state == UAVCAN_TASK_STATE_RUNNING &&
            ctx->tx_task_state == UAVCAN_TASK_STATE_RUNNING &&
            ctx->rx_task_state == UAVCAN_TASK_STATE_RUNNING);
}

/**
 * @brief Reset task context to default values
 */
void uavcanTasksReset(UavcanTaskContext* ctx) {
    if (ctx == NULL) {
        return;
    }

    // Clear the entire structure
    memset(ctx, 0, sizeof(UavcanTaskContext));

    // Set default values
    ctx->node_task_state = UAVCAN_TASK_STATE_STOPPED;
    ctx->tx_task_state = UAVCAN_TASK_STATE_STOPPED;
    ctx->rx_task_state = UAVCAN_TASK_STATE_STOPPED;
    ctx->tasks_started = false;
}

/**
 * @brief Get task status as formatted string
 */
size_t uavcanTasksGetStatusString(const UavcanTaskContext* ctx, 
                                 char* buffer, 
                                 size_t buffer_size) {
    if (ctx == NULL || buffer == NULL || buffer_size == 0) {
        return 0;
    }

    return snprintf(buffer, buffer_size,
                   "UAVCAN Tasks Status:\n"
                   "  Node Task: %s (cycles: %lu)\n"
                   "  TX Task: %s (cycles: %lu)\n"
                   "  RX Task: %s (cycles: %lu)\n"
                   "  Tasks Started: %s",
                   taskStateToString(ctx->node_task_state),
                   (unsigned long)ctx->node_task_cycles,
                   taskStateToString(ctx->tx_task_state),
                   (unsigned long)ctx->tx_task_cycles,
                   taskStateToString(ctx->rx_task_state),
                   (unsigned long)ctx->rx_task_cycles,
                   ctx->tasks_started ? "Yes" : "No");
}

// Task Implementations

/**
 * @brief UAVCAN Node Task - Main node management and lifecycle
 */
static void uavcanNodeTask(void* pvParameters) {
    UavcanTaskContext* ctx = (UavcanTaskContext*)pvParameters;
    UavcanTaskCommandMessage cmd;
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t cycle_time = pdMS_TO_TICKS(100); // 10Hz cycle
    
    UAVCAN_INFO_PRINT("UAVCAN Node Task started");
    updateTaskState(ctx, &ctx->node_task_state, UAVCAN_TASK_STATE_INITIALIZING);

    // Initialize node if not already done
    if (!uavcanNodeIsInitialized(ctx->node_context)) {
        error_t result = uavcanNodeStart(ctx->node_context);
        if (UAVCAN_FAILED(result)) {
            UAVCAN_ERROR_PRINT("Failed to start UAVCAN node: %d", result);
            updateTaskState(ctx, &ctx->node_task_state, UAVCAN_TASK_STATE_ERROR);
            vTaskDelete(NULL);
            return;
        }
    }

    updateTaskState(ctx, &ctx->node_task_state, UAVCAN_TASK_STATE_RUNNING);

    while (1) {
        ctx->node_task_cycles++;

        // Process commands from queue (non-blocking)
        while (xQueueReceive(ctx->node_command_queue, &cmd, 0) == pdTRUE) {
            error_t result = processNodeCommand(ctx, &cmd);
            if (UAVCAN_FAILED(result)) {
                UAVCAN_ERROR_PRINT("Failed to process node command: %d", result);
            }

            // Check if we should stop
            if (cmd.command == UAVCAN_TASK_CMD_STOP) {
                updateTaskState(ctx, &ctx->node_task_state, UAVCAN_TASK_STATE_STOPPING);
                goto task_exit;
            }
        }

        // Update node uptime
        uavcanNodeUpdateUptime(ctx->node_context);

        // Process dynamic node ID allocation if needed
        if (uavcanNodeGetId(ctx->node_context) == UAVCAN_NODE_ID_UNSET) {
            uavcanNodeProcessDynamicAllocation(ctx->node_context);
        }

        // Periodic health check
        UavcanNodeHealth current_health = uavcanNodeGetHealth(ctx->node_context);
        if (current_health == UAVCAN_NODE_HEALTH_WARNING) {
            // Could implement health recovery logic here
        }

        // Wait for next cycle
        vTaskDelayUntil(&last_wake_time, cycle_time);
    }

task_exit:
    UAVCAN_INFO_PRINT("UAVCAN Node Task stopping");
    uavcanNodeStop(ctx->node_context);
    updateTaskState(ctx, &ctx->node_task_state, UAVCAN_TASK_STATE_STOPPED);
    vTaskDelete(NULL);
}

/**
 * @brief UAVCAN TX Task - Priority-based message transmission
 */
static void uavcanTxTask(void* pvParameters) {
    UavcanTaskContext* ctx = (UavcanTaskContext*)pvParameters;
    UavcanMessage message;
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t cycle_time = pdMS_TO_TICKS(10); // 100Hz cycle for responsive transmission
    
    UAVCAN_INFO_PRINT("UAVCAN TX Task started");
    updateTaskState(ctx, &ctx->tx_task_state, UAVCAN_TASK_STATE_INITIALIZING);

    // Wait for node task to be running
    while (ctx->node_task_state != UAVCAN_TASK_STATE_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (ctx->node_task_state == UAVCAN_TASK_STATE_ERROR ||
            ctx->node_task_state == UAVCAN_TASK_STATE_STOPPED) {
            UAVCAN_ERROR_PRINT("Node task not running, TX task exiting");
            updateTaskState(ctx, &ctx->tx_task_state, UAVCAN_TASK_STATE_ERROR);
            vTaskDelete(NULL);
            return;
        }
    }

    updateTaskState(ctx, &ctx->tx_task_state, UAVCAN_TASK_STATE_RUNNING);

    while (1) {
        ctx->tx_task_cycles++;

        // Check if we should stop
        if (ctx->node_task_state == UAVCAN_TASK_STATE_STOPPING ||
            ctx->node_task_state == UAVCAN_TASK_STATE_STOPPED) {
            break;
        }

        // Process priority queue - get highest priority message
        error_t result = uavcanPriorityQueuePop((UavcanPriorityQueue*)ctx->priority_queue, &message);
        if (UAVCAN_SUCCEEDED(result)) {
            // Transmit message via UDP transport with retry logic
            const uint8_t max_retries = 3;
            uint8_t retry_count = 0;
            bool transmission_successful = false;
            
            while (retry_count < max_retries && !transmission_successful) {
                result = uavcanUdpTransportSend((UavcanUdpTransport*)ctx->udp_transport, 
                                              message.payload, 
                                              message.payload_size);
                if (UAVCAN_SUCCEEDED(result)) {
                    transmission_successful = true;
                    UAVCAN_DEBUG_PRINT("Message transmitted successfully (attempt %d)", retry_count + 1);
                } else {
                    retry_count++;
                    UAVCAN_WARN_PRINT("Transmission failed (attempt %d/%d): %d", 
                                     retry_count, max_retries, result);
                    
                    if (retry_count < max_retries) {
                        // Brief delay before retry
                        vTaskDelay(pdMS_TO_TICKS(10));
                    }
                }
            }
            
            if (!transmission_successful) {
                UAVCAN_ERROR_PRINT("Failed to transmit message after %d attempts", max_retries);
                // Update error statistics or take other recovery action
                uavcanNodeSetHealth(ctx->node_context, UAVCAN_NODE_HEALTH_ADVISORY);
            }
        }

        // Wait for next cycle
        vTaskDelayUntil(&last_wake_time, cycle_time);
    }

    UAVCAN_INFO_PRINT("UAVCAN TX Task stopping");
    updateTaskState(ctx, &ctx->tx_task_state, UAVCAN_TASK_STATE_STOPPED);
    vTaskDelete(NULL);
}

/**
 * @brief UAVCAN RX Task - Message reception and processing
 */
static void uavcanRxTask(void* pvParameters) {
    UavcanTaskContext* ctx = (UavcanTaskContext*)pvParameters;
    UavcanMessage message;
    TickType_t last_wake_time = xTaskGetTickCount();
    const TickType_t cycle_time = pdMS_TO_TICKS(10); // 100Hz cycle for responsive reception
    
    UAVCAN_INFO_PRINT("UAVCAN RX Task started");
    updateTaskState(ctx, &ctx->rx_task_state, UAVCAN_TASK_STATE_INITIALIZING);

    // Wait for node task to be running
    while (ctx->node_task_state != UAVCAN_TASK_STATE_RUNNING) {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (ctx->node_task_state == UAVCAN_TASK_STATE_ERROR ||
            ctx->node_task_state == UAVCAN_TASK_STATE_STOPPED) {
            UAVCAN_ERROR_PRINT("Node task not running, RX task exiting");
            updateTaskState(ctx, &ctx->rx_task_state, UAVCAN_TASK_STATE_ERROR);
            vTaskDelete(NULL);
            return;
        }
    }

    updateTaskState(ctx, &ctx->rx_task_state, UAVCAN_TASK_STATE_RUNNING);

    while (1) {
        ctx->rx_task_cycles++;

        // Check if we should stop
        if (ctx->node_task_state == UAVCAN_TASK_STATE_STOPPING ||
            ctx->node_task_state == UAVCAN_TASK_STATE_STOPPED) {
            break;
        }

        // Receive message from UDP transport
        size_t received_size = 0;
        uint8_t receive_buffer[UAVCAN_MAX_PAYLOAD_SIZE];
        
        error_t result = uavcanUdpTransportReceive((UavcanUdpTransport*)ctx->udp_transport,
                                                 receive_buffer,
                                                 &received_size);
        if (UAVCAN_SUCCEEDED(result) && received_size > 0) {
            // Parse and validate received message
            result = uavcanMessageReceive(&message, pdMS_TO_TICKS(0));
            if (UAVCAN_SUCCEEDED(result)) {
                // Validate message parameters
                if (!uavcanIsValidPriority(message.priority)) {
                    UAVCAN_ERROR_PRINT("Received message with invalid priority: %d", message.priority);
                    continue;
                }
                
                if (!uavcanIsValidSubjectId(message.subject_id)) {
                    UAVCAN_ERROR_PRINT("Received message with invalid subject ID: %lu", 
                                      (unsigned long)message.subject_id);
                    continue;
                }
                
                // Route message to appropriate handler based on subject ID
                if (message.subject_id == 7509) { // Heartbeat subject ID
                    UAVCAN_DEBUG_PRINT("Received heartbeat from node %d", message.source_node_id);
                    // Could update node discovery table here
                } else if (message.subject_id >= 8184 && message.subject_id <= 8191) { // Dynamic node ID allocation
                    UAVCAN_DEBUG_PRINT("Received dynamic node ID allocation message");
                    // Forward to dynamic allocation handler
                } else {
                    UAVCAN_DEBUG_PRINT("Received message: Subject ID %lu, Priority %d, Size %zu, Source %d",
                                      (unsigned long)message.subject_id,
                                      message.priority,
                                      message.payload_size,
                                      message.source_node_id);
                }
                
                // Update receive statistics (this would be part of a statistics structure)
                // For now, just increment cycle count as a basic statistic
                
            } else {
                UAVCAN_ERROR_PRINT("Failed to parse received message: %d", result);
                // Update error statistics
                // Could implement error recovery or message discarding here
            }
        } else if (UAVCAN_FAILED(result) && result != UAVCAN_ERROR_TIMEOUT) {
            // Only log non-timeout errors to avoid spam
            UAVCAN_ERROR_PRINT("Failed to receive message: %d", result);
            // Could implement error recovery here
        }

        // Wait for next cycle
        vTaskDelayUntil(&last_wake_time, cycle_time);
    }

    UAVCAN_INFO_PRINT("UAVCAN RX Task stopping");
    updateTaskState(ctx, &ctx->rx_task_state, UAVCAN_TASK_STATE_STOPPED);
    vTaskDelete(NULL);
}

// Helper Functions

/**
 * @brief Process node command
 */
static error_t processNodeCommand(UavcanTaskContext* ctx, const UavcanTaskCommandMessage* cmd) {
    if (ctx == NULL || cmd == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    switch (cmd->command) {
        case UAVCAN_TASK_CMD_START:
            UAVCAN_INFO_PRINT("Processing START command");
            return uavcanNodeStart(ctx->node_context);

        case UAVCAN_TASK_CMD_STOP:
            UAVCAN_INFO_PRINT("Processing STOP command");
            return UAVCAN_ERROR_NONE; // Handled in main task loop

        case UAVCAN_TASK_CMD_RESTART:
            UAVCAN_INFO_PRINT("Processing RESTART command");
            uavcanNodeStop(ctx->node_context);
            vTaskDelay(pdMS_TO_TICKS(100));
            return uavcanNodeStart(ctx->node_context);

        case UAVCAN_TASK_CMD_HEALTH_CHECK:
            UAVCAN_INFO_PRINT("Processing HEALTH_CHECK command");
            // Could implement comprehensive health check here
            return UAVCAN_ERROR_NONE;

        case UAVCAN_TASK_CMD_UPDATE_CONFIG:
            UAVCAN_INFO_PRINT("Processing UPDATE_CONFIG command");
            // Could implement configuration update here
            return UAVCAN_ERROR_NONE;

        default:
            UAVCAN_ERROR_PRINT("Unknown command: %d", cmd->command);
            return UAVCAN_ERROR_INVALID_PARAMETER;
    }
}

/**
 * @brief Update task state with thread safety
 */
static void updateTaskState(UavcanTaskContext* ctx, UavcanTaskState* state, UavcanTaskState new_state) {
    if (ctx == NULL || state == NULL) {
        return;
    }

    if (xSemaphoreTake(ctx->state_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        *state = new_state;
        xSemaphoreGive(ctx->state_mutex);
    }
}

/**
 * @brief Convert task state to string
 */
static const char* taskStateToString(UavcanTaskState state) {
    switch (state) {
        case UAVCAN_TASK_STATE_STOPPED:      return "Stopped";
        case UAVCAN_TASK_STATE_INITIALIZING: return "Initializing";
        case UAVCAN_TASK_STATE_RUNNING:      return "Running";
        case UAVCAN_TASK_STATE_ERROR:        return "Error";
        case UAVCAN_TASK_STATE_STOPPING:     return "Stopping";
        default:                             return "Unknown";
    }
}
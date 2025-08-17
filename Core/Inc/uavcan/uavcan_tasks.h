#ifndef UAVCAN_TASKS_H
#define UAVCAN_TASKS_H

#include "uavcan_common.h"
#include "uavcan_types.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

#ifdef __cplusplus
extern "C" {
#endif

// Task Configuration Constants
#define UAVCAN_NODE_TASK_STACK_SIZE         (configMINIMAL_STACK_SIZE * 4)
#define UAVCAN_TX_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE * 3)
#define UAVCAN_RX_TASK_STACK_SIZE           (configMINIMAL_STACK_SIZE * 3)

#define UAVCAN_NODE_TASK_PRIORITY           (tskIDLE_PRIORITY + 4)  // Medium-High
#define UAVCAN_TX_TASK_PRIORITY             (tskIDLE_PRIORITY + 4)  // Medium-High
#define UAVCAN_RX_TASK_PRIORITY             (tskIDLE_PRIORITY + 3)  // Medium

// Task State Machine States
typedef enum {
    UAVCAN_TASK_STATE_STOPPED = 0,
    UAVCAN_TASK_STATE_INITIALIZING,
    UAVCAN_TASK_STATE_RUNNING,
    UAVCAN_TASK_STATE_ERROR,
    UAVCAN_TASK_STATE_STOPPING
} UavcanTaskState;

// Task Context Structure
typedef struct {
    TaskHandle_t node_task_handle;
    TaskHandle_t tx_task_handle;
    TaskHandle_t rx_task_handle;
    
    UavcanTaskState node_task_state;
    UavcanTaskState tx_task_state;
    UavcanTaskState rx_task_state;
    
    SemaphoreHandle_t state_mutex;
    QueueHandle_t node_command_queue;
    QueueHandle_t tx_message_queue;
    QueueHandle_t rx_message_queue;
    
    UavcanNodeContext* node_context;
    void* priority_queue;      // UavcanPriorityQueue*
    void* udp_transport;       // UavcanUdpTransport*
    
    bool tasks_started;
    uint32_t node_task_cycles;
    uint32_t tx_task_cycles;
    uint32_t rx_task_cycles;
} UavcanTaskContext;

// Task Command Types
typedef enum {
    UAVCAN_TASK_CMD_START = 0,
    UAVCAN_TASK_CMD_STOP,
    UAVCAN_TASK_CMD_RESTART,
    UAVCAN_TASK_CMD_UPDATE_CONFIG,
    UAVCAN_TASK_CMD_HEALTH_CHECK
} UavcanTaskCommand;

// Task Command Structure
typedef struct {
    UavcanTaskCommand command;
    void* data;
    size_t data_size;
} UavcanTaskCommandMessage;

/**
 * @brief Initialize UAVCAN task architecture
 * @param ctx Pointer to task context structure
 * @param node_ctx Pointer to node context
 * @param priority_queue Pointer to priority queue
 * @param udp_transport Pointer to UDP transport
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanTasksInit(UavcanTaskContext* ctx, 
                       UavcanNodeContext* node_ctx,
                       void* priority_queue,
                       void* udp_transport);

/**
 * @brief Start all UAVCAN tasks
 * @param ctx Pointer to task context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanTasksStart(UavcanTaskContext* ctx);

/**
 * @brief Stop all UAVCAN tasks
 * @param ctx Pointer to task context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanTasksStop(UavcanTaskContext* ctx);

/**
 * @brief Send command to node task
 * @param ctx Pointer to task context
 * @param command Command to send
 * @param data Optional command data
 * @param data_size Size of command data
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanTasksSendCommand(UavcanTaskContext* ctx,
                              UavcanTaskCommand command,
                              void* data,
                              size_t data_size);

/**
 * @brief Get task states
 * @param ctx Pointer to task context
 * @param node_state Pointer to store node task state
 * @param tx_state Pointer to store TX task state
 * @param rx_state Pointer to store RX task state
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanTasksGetStates(const UavcanTaskContext* ctx,
                            UavcanTaskState* node_state,
                            UavcanTaskState* tx_state,
                            UavcanTaskState* rx_state);

/**
 * @brief Get task statistics
 * @param ctx Pointer to task context
 * @param node_cycles Pointer to store node task cycle count
 * @param tx_cycles Pointer to store TX task cycle count
 * @param rx_cycles Pointer to store RX task cycle count
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
error_t uavcanTasksGetStatistics(const UavcanTaskContext* ctx,
                                uint32_t* node_cycles,
                                uint32_t* tx_cycles,
                                uint32_t* rx_cycles);

/**
 * @brief Check if all tasks are running
 * @param ctx Pointer to task context
 * @return true if all tasks are running, false otherwise
 */
bool uavcanTasksAreRunning(const UavcanTaskContext* ctx);

/**
 * @brief Reset task context to default values
 * @param ctx Pointer to task context
 */
void uavcanTasksReset(UavcanTaskContext* ctx);

/**
 * @brief Get task status as formatted string
 * @param ctx Pointer to task context
 * @param buffer Buffer to store status string
 * @param buffer_size Size of buffer
 * @return Number of characters written to buffer
 */
size_t uavcanTasksGetStatusString(const UavcanTaskContext* ctx, 
                                 char* buffer, 
                                 size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_TASKS_H
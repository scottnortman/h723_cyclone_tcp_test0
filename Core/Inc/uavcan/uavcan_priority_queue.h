#ifndef UAVCAN_PRIORITY_QUEUE_H
#define UAVCAN_PRIORITY_QUEUE_H

#include "uavcan_types.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Priority Queue Configuration
#define UAVCAN_PRIORITY_QUEUE_DEPTH_DEFAULT 16
#define UAVCAN_PRIORITY_QUEUE_DEPTH_HIGH    32  // For higher priority levels
#define UAVCAN_PRIORITY_QUEUE_DEPTH_LOW     8   // For lower priority levels

// Priority Queue Statistics
typedef struct {
    uint32_t messages_queued;
    uint32_t messages_dequeued;
    uint32_t overflow_count;
    uint32_t current_depth;
    uint32_t max_depth_reached;
} UavcanPriorityQueueStats;

// Enhanced Priority Queue Structure
typedef struct {
    QueueHandle_t priority_queues[CYPHAL_PRIORITY_LEVELS];
    SemaphoreHandle_t queue_mutex;
    uint32_t queue_depths[CYPHAL_PRIORITY_LEVELS];
    uint32_t overflow_counts[CYPHAL_PRIORITY_LEVELS];
    UavcanPriorityQueueStats stats[CYPHAL_PRIORITY_LEVELS];
    bool initialized;
} UavcanPriorityQueue;

/**
 * @brief Initialize the UAVCAN priority queue system
 * @param pq Pointer to the priority queue structure
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueueInit(UavcanPriorityQueue* pq);

/**
 * @brief Deinitialize the UAVCAN priority queue system
 * @param pq Pointer to the priority queue structure
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueueDeinit(UavcanPriorityQueue* pq);

/**
 * @brief Push a message to the appropriate priority queue
 * @param pq Pointer to the priority queue structure
 * @param msg Pointer to the message to queue
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueuePush(UavcanPriorityQueue* pq, const UavcanMessage* msg);

/**
 * @brief Pop the highest priority message from the queues
 * @param pq Pointer to the priority queue structure
 * @param msg Pointer to store the retrieved message
 * @param timeout_ms Timeout in milliseconds (0 for no wait, portMAX_DELAY for infinite)
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueuePop(UavcanPriorityQueue* pq, UavcanMessage* msg, uint32_t timeout_ms);

/**
 * @brief Get the next available priority level with messages
 * @param pq Pointer to the priority queue structure
 * @return Priority level (0-7) or CYPHAL_PRIORITY_LEVELS if no messages
 */
uint8_t uavcanPriorityQueueGetNextPriority(const UavcanPriorityQueue* pq);

/**
 * @brief Validate priority level
 * @param priority Priority level to validate
 * @return true if valid, false otherwise
 */
bool uavcanPriorityQueueValidatePriority(uint8_t priority);

/**
 * @brief Get statistics for a specific priority level
 * @param pq Pointer to the priority queue structure
 * @param priority Priority level (0-7)
 * @param stats Pointer to store statistics
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueueGetStats(const UavcanPriorityQueue* pq, uint8_t priority, 
                                       UavcanPriorityQueueStats* stats);

/**
 * @brief Reset statistics for all priority levels
 * @param pq Pointer to the priority queue structure
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueueResetStats(UavcanPriorityQueue* pq);

/**
 * @brief Get total number of messages across all priority queues
 * @param pq Pointer to the priority queue structure
 * @return Total number of queued messages
 */
uint32_t uavcanPriorityQueueGetTotalCount(const UavcanPriorityQueue* pq);

/**
 * @brief Check if any priority queue has messages
 * @param pq Pointer to the priority queue structure
 * @return true if any queue has messages, false otherwise
 */
bool uavcanPriorityQueueHasMessages(const UavcanPriorityQueue* pq);

/**
 * @brief Flush all messages from all priority queues
 * @param pq Pointer to the priority queue structure
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueueFlushAll(UavcanPriorityQueue* pq);

/**
 * @brief Flush messages from a specific priority queue
 * @param pq Pointer to the priority queue structure
 * @param priority Priority level to flush (0-7)
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueueFlushPriority(UavcanPriorityQueue* pq, uint8_t priority);

/**
 * @brief Get the number of messages in a specific priority queue
 * @param pq Pointer to the priority queue structure
 * @param priority Priority level (0-7)
 * @return Number of messages in the queue, 0 if invalid parameters
 */
uint32_t uavcanPriorityQueueGetPriorityCount(const UavcanPriorityQueue* pq, uint8_t priority);

/**
 * @brief Try to push a message with timeout (non-blocking with retry)
 * @param pq Pointer to the priority queue structure
 * @param msg Pointer to the message to queue
 * @param timeout_ms Timeout in milliseconds for mutex acquisition
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueuePushWithTimeout(UavcanPriorityQueue* pq, const UavcanMessage* msg, uint32_t timeout_ms);

/**
 * @brief Get comprehensive statistics for all priority levels
 * @param pq Pointer to the priority queue structure
 * @param total_stats Array to store statistics for all priority levels
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanPriorityQueueGetAllStats(const UavcanPriorityQueue* pq, UavcanPriorityQueueStats total_stats[CYPHAL_PRIORITY_LEVELS]);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_PRIORITY_QUEUE_H
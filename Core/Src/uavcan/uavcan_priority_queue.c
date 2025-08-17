#include "uavcan/uavcan_priority_queue.h"
#include "uavcan/uavcan_types.h"
#include <string.h>

// Static function declarations
static uint32_t getPriorityQueueDepth(uint8_t priority);
static void updateQueueStats(UavcanPriorityQueue* pq, uint8_t priority, bool is_push, bool overflow);

/**
 * @brief Get the appropriate queue depth for a priority level
 * @param priority Priority level (0-7)
 * @return Queue depth
 */
static uint32_t getPriorityQueueDepth(uint8_t priority)
{
    // Higher priority levels get larger queues
    switch (priority) {
        case CYPHAL_PRIORITY_EXCEPTIONAL:
        case CYPHAL_PRIORITY_IMMEDIATE:
        case CYPHAL_PRIORITY_FAST:
            return UAVCAN_PRIORITY_QUEUE_DEPTH_HIGH;
        case CYPHAL_PRIORITY_HIGH:
        case CYPHAL_PRIORITY_NOMINAL:
            return UAVCAN_PRIORITY_QUEUE_DEPTH_DEFAULT;
        case CYPHAL_PRIORITY_LOW:
        case CYPHAL_PRIORITY_SLOW:
        case CYPHAL_PRIORITY_OPTIONAL:
            return UAVCAN_PRIORITY_QUEUE_DEPTH_LOW;
        default:
            return UAVCAN_PRIORITY_QUEUE_DEPTH_DEFAULT;
    }
}

/**
 * @brief Update queue statistics
 * @param pq Pointer to the priority queue structure
 * @param priority Priority level
 * @param is_push True for push operation, false for pop
 * @param overflow True if overflow occurred
 */
static void updateQueueStats(UavcanPriorityQueue* pq, uint8_t priority, bool is_push, bool overflow)
{
    if (!pq || priority >= CYPHAL_PRIORITY_LEVELS) {
        return;
    }

    UavcanPriorityQueueStats* stats = &pq->stats[priority];
    
    if (is_push) {
        if (overflow) {
            stats->overflow_count++;
            pq->overflow_counts[priority]++;
        } else {
            stats->messages_queued++;
        }
    } else {
        stats->messages_dequeued++;
    }

    // Update current depth
    if (pq->priority_queues[priority] != NULL) {
        stats->current_depth = uxQueueMessagesWaiting(pq->priority_queues[priority]);
        if (stats->current_depth > stats->max_depth_reached) {
            stats->max_depth_reached = stats->current_depth;
        }
    }
}

UavcanError uavcanPriorityQueueInit(UavcanPriorityQueue* pq)
{
    if (!pq) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize structure
    memset(pq, 0, sizeof(UavcanPriorityQueue));

    // Create mutex for thread safety
    pq->queue_mutex = xSemaphoreCreateMutex();
    if (!pq->queue_mutex) {
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    // Create priority queues
    for (uint8_t i = 0; i < CYPHAL_PRIORITY_LEVELS; i++) {
        uint32_t depth = getPriorityQueueDepth(i);
        pq->queue_depths[i] = depth;
        
        // Create queue for UavcanMessage pointers
        pq->priority_queues[i] = xQueueCreate(depth, sizeof(UavcanMessage));
        if (!pq->priority_queues[i]) {
            // Cleanup already created queues
            uavcanPriorityQueueDeinit(pq);
            return UAVCAN_ERROR_MEMORY_ALLOCATION;
        }
    }

    pq->initialized = true;
    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanPriorityQueueDeinit(UavcanPriorityQueue* pq)
{
    if (!pq) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Delete all queues
    for (uint8_t i = 0; i < CYPHAL_PRIORITY_LEVELS; i++) {
        if (pq->priority_queues[i]) {
            vQueueDelete(pq->priority_queues[i]);
            pq->priority_queues[i] = NULL;
        }
    }

    // Delete mutex
    if (pq->queue_mutex) {
        vSemaphoreDelete(pq->queue_mutex);
        pq->queue_mutex = NULL;
    }

    pq->initialized = false;
    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanPriorityQueuePush(UavcanPriorityQueue* pq, const UavcanMessage* msg)
{
    if (!pq || !msg) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!pq->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    if (!uavcanPriorityQueueValidatePriority(msg->priority)) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Take mutex for thread safety
    if (xSemaphoreTake(pq->queue_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    UavcanError result = UAVCAN_ERROR_NONE;
    uint8_t priority = msg->priority;

    // Try to send to the appropriate priority queue
    if (xQueueSend(pq->priority_queues[priority], msg, 0) != pdTRUE) {
        // Queue is full - handle overflow
        updateQueueStats(pq, priority, true, true);
        result = UAVCAN_ERROR_QUEUE_FULL;
    } else {
        // Successfully queued
        updateQueueStats(pq, priority, true, false);
    }

    // Release mutex
    xSemaphoreGive(pq->queue_mutex);
    return result;
}

UavcanError uavcanPriorityQueuePop(UavcanPriorityQueue* pq, UavcanMessage* msg, uint32_t timeout_ms)
{
    if (!pq || !msg) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!pq->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Take mutex for thread safety
    TickType_t timeout_ticks = (timeout_ms == 0) ? 0 : 
                              (timeout_ms == UINT32_MAX) ? portMAX_DELAY : 
                              pdMS_TO_TICKS(timeout_ms);

    if (xSemaphoreTake(pq->queue_mutex, timeout_ticks) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    UavcanError result = UAVCAN_ERROR_TIMEOUT;

    // Check queues in priority order (0 = highest priority)
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        if (uxQueueMessagesWaiting(pq->priority_queues[priority]) > 0) {
            if (xQueueReceive(pq->priority_queues[priority], msg, 0) == pdTRUE) {
                updateQueueStats(pq, priority, false, false);
                result = UAVCAN_ERROR_NONE;
                break;
            }
        }
    }

    // Release mutex
    xSemaphoreGive(pq->queue_mutex);
    return result;
}

uint8_t uavcanPriorityQueueGetNextPriority(const UavcanPriorityQueue* pq)
{
    if (!pq || !pq->initialized) {
        return CYPHAL_PRIORITY_LEVELS;
    }

    // Check queues in priority order (0 = highest priority)
    for (uint8_t priority = 0; priority < CYPHAL_PRIORITY_LEVELS; priority++) {
        if (pq->priority_queues[priority] && 
            uxQueueMessagesWaiting(pq->priority_queues[priority]) > 0) {
            return priority;
        }
    }

    return CYPHAL_PRIORITY_LEVELS; // No messages available
}

bool uavcanPriorityQueueValidatePriority(uint8_t priority)
{
    return (priority < CYPHAL_PRIORITY_LEVELS);
}

UavcanError uavcanPriorityQueueGetStats(const UavcanPriorityQueue* pq, uint8_t priority, 
                                       UavcanPriorityQueueStats* stats)
{
    if (!pq || !stats || !uavcanPriorityQueueValidatePriority(priority)) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!pq->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Copy statistics
    *stats = pq->stats[priority];

    // Update current depth
    if (pq->priority_queues[priority]) {
        stats->current_depth = uxQueueMessagesWaiting(pq->priority_queues[priority]);
    }

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanPriorityQueueResetStats(UavcanPriorityQueue* pq)
{
    if (!pq) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!pq->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Take mutex for thread safety
    if (xSemaphoreTake(pq->queue_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Reset all statistics
    for (uint8_t i = 0; i < CYPHAL_PRIORITY_LEVELS; i++) {
        memset(&pq->stats[i], 0, sizeof(UavcanPriorityQueueStats));
        pq->overflow_counts[i] = 0;
    }

    // Release mutex
    xSemaphoreGive(pq->queue_mutex);
    return UAVCAN_ERROR_NONE;
}

uint32_t uavcanPriorityQueueGetTotalCount(const UavcanPriorityQueue* pq)
{
    if (!pq || !pq->initialized) {
        return 0;
    }

    uint32_t total = 0;
    for (uint8_t i = 0; i < CYPHAL_PRIORITY_LEVELS; i++) {
        if (pq->priority_queues[i]) {
            total += uxQueueMessagesWaiting(pq->priority_queues[i]);
        }
    }

    return total;
}

bool uavcanPriorityQueueHasMessages(const UavcanPriorityQueue* pq)
{
    return (uavcanPriorityQueueGetTotalCount(pq) > 0);
}

UavcanError uavcanPriorityQueueFlushAll(UavcanPriorityQueue* pq)
{
    if (!pq) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!pq->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Take mutex for thread safety
    if (xSemaphoreTake(pq->queue_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Flush all queues
    for (uint8_t i = 0; i < CYPHAL_PRIORITY_LEVELS; i++) {
        if (pq->priority_queues[i]) {
            xQueueReset(pq->priority_queues[i]);
        }
    }

    // Release mutex
    xSemaphoreGive(pq->queue_mutex);
    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanPriorityQueueFlushPriority(UavcanPriorityQueue* pq, uint8_t priority)
{
    if (!pq || !uavcanPriorityQueueValidatePriority(priority)) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!pq->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Take mutex for thread safety
    if (xSemaphoreTake(pq->queue_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Flush specific priority queue
    if (pq->priority_queues[priority]) {
        xQueueReset(pq->priority_queues[priority]);
    }

    // Release mutex
    xSemaphoreGive(pq->queue_mutex);
    return UAVCAN_ERROR_NONE;
}

uint32_t uavcanPriorityQueueGetPriorityCount(const UavcanPriorityQueue* pq, uint8_t priority)
{
    if (!pq || !pq->initialized || !uavcanPriorityQueueValidatePriority(priority)) {
        return 0;
    }

    if (pq->priority_queues[priority]) {
        return uxQueueMessagesWaiting(pq->priority_queues[priority]);
    }

    return 0;
}

UavcanError uavcanPriorityQueuePushWithTimeout(UavcanPriorityQueue* pq, const UavcanMessage* msg, uint32_t timeout_ms)
{
    if (!pq || !msg) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!pq->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    if (!uavcanPriorityQueueValidatePriority(msg->priority)) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Take mutex with specified timeout
    TickType_t timeout_ticks = (timeout_ms == 0) ? 0 : 
                              (timeout_ms == UINT32_MAX) ? portMAX_DELAY : 
                              pdMS_TO_TICKS(timeout_ms);

    if (xSemaphoreTake(pq->queue_mutex, timeout_ticks) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    UavcanError result = UAVCAN_ERROR_NONE;
    uint8_t priority = msg->priority;

    // Try to send to the appropriate priority queue with timeout
    if (xQueueSend(pq->priority_queues[priority], msg, timeout_ticks) != pdTRUE) {
        // Queue is full or timeout occurred
        updateQueueStats(pq, priority, true, true);
        result = UAVCAN_ERROR_QUEUE_FULL;
    } else {
        // Successfully queued
        updateQueueStats(pq, priority, true, false);
    }

    // Release mutex
    xSemaphoreGive(pq->queue_mutex);
    return result;
}

UavcanError uavcanPriorityQueueGetAllStats(const UavcanPriorityQueue* pq, UavcanPriorityQueueStats total_stats[CYPHAL_PRIORITY_LEVELS])
{
    if (!pq || !total_stats) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!pq->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Copy statistics for all priority levels
    for (uint8_t i = 0; i < CYPHAL_PRIORITY_LEVELS; i++) {
        total_stats[i] = pq->stats[i];
        
        // Update current depth
        if (pq->priority_queues[i]) {
            total_stats[i].current_depth = uxQueueMessagesWaiting(pq->priority_queues[i]);
        }
    }

    return UAVCAN_ERROR_NONE;
}
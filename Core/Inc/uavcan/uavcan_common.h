#ifndef UAVCAN_COMMON_H
#define UAVCAN_COMMON_H

#include "uavcan_types.h"
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// UAVCAN Version Information
#define UAVCAN_VERSION_MAJOR    1
#define UAVCAN_VERSION_MINOR    0
#define UAVCAN_VERSION_PATCH    0

// Common Return Type (avoid conflict with system error_t)
#ifndef ERROR_H
typedef UavcanError error_t;
#endif

// Success/Error Macros
#define UAVCAN_SUCCESS          UAVCAN_ERROR_NONE
#define UAVCAN_FAILED(err)      ((err) != UAVCAN_ERROR_NONE)
#define UAVCAN_SUCCEEDED(err)   ((err) == UAVCAN_ERROR_NONE)

// Validation Macros
#define UAVCAN_IS_VALID_NODE_ID(id)     ((id) >= UAVCAN_NODE_ID_MIN && (id) <= UAVCAN_NODE_ID_MAX)
#define UAVCAN_IS_VALID_PRIORITY(prio)  ((prio) < CYPHAL_PRIORITY_LEVELS)
#define UAVCAN_IS_VALID_SUBJECT_ID(id)  ((id) <= UAVCAN_SUBJECT_ID_MAX)
#define UAVCAN_IS_VALID_SERVICE_ID(id)  ((id) <= UAVCAN_SERVICE_ID_MAX)

// Timeout Constants
#define UAVCAN_TIMEOUT_INFINITE         0xFFFFFFFF
#define UAVCAN_TIMEOUT_DEFAULT_MS       1000
#define UAVCAN_TIMEOUT_HEARTBEAT_MS     5000
#define UAVCAN_TIMEOUT_NODE_DISCOVERY   10000

// Buffer Size Constants
#define UAVCAN_MAX_NODE_NAME_LENGTH     63
#define UAVCAN_MAX_NODES                128
#define UAVCAN_DEFAULT_QUEUE_DEPTH      16
#define UAVCAN_HIGH_PRIORITY_QUEUE_DEPTH 32

// Logging Levels
typedef enum {
    UAVCAN_LOG_LEVEL_NONE = 0,
    UAVCAN_LOG_LEVEL_ERROR,
    UAVCAN_LOG_LEVEL_WARNING,
    UAVCAN_LOG_LEVEL_INFO,
    UAVCAN_LOG_LEVEL_DEBUG,
    UAVCAN_LOG_LEVEL_TRACE
} UavcanLogLevel;

// Error to String Conversion
const char* uavcanErrorToString(UavcanError error);

// Node Health to String Conversion
const char* uavcanNodeHealthToString(UavcanNodeHealth health);

// Node Mode to String Conversion
const char* uavcanNodeModeToString(UavcanNodeMode mode);

// Priority to String Conversion
const char* uavcanPriorityToString(uint8_t priority);

// Utility Functions
static inline bool uavcanIsValidNodeId(uint8_t node_id) {
    return UAVCAN_IS_VALID_NODE_ID(node_id);
}

static inline bool uavcanIsValidPriority(uint8_t priority) {
    return UAVCAN_IS_VALID_PRIORITY(priority);
}

static inline bool uavcanIsValidSubjectId(uint32_t subject_id) {
    return UAVCAN_IS_VALID_SUBJECT_ID(subject_id);
}

static inline bool uavcanIsValidServiceId(uint16_t service_id) {
    return UAVCAN_IS_VALID_SERVICE_ID(service_id);
}

// Memory Management Helpers
#define UAVCAN_MALLOC(size)     malloc(size)
#define UAVCAN_FREE(ptr)        free(ptr)
#define UAVCAN_MEMSET(ptr, val, size) memset(ptr, val, size)
#define UAVCAN_MEMCPY(dst, src, size) memcpy(dst, src, size)

// Debug and Logging Macros (can be disabled in release builds)
#ifdef UAVCAN_DEBUG_ENABLED
    #define UAVCAN_DEBUG_PRINT(fmt, ...) printf("[UAVCAN DEBUG] " fmt "\n", ##__VA_ARGS__)
    #define UAVCAN_INFO_PRINT(fmt, ...)  printf("[UAVCAN INFO] " fmt "\n", ##__VA_ARGS__)
    #define UAVCAN_WARN_PRINT(fmt, ...)  printf("[UAVCAN WARN] " fmt "\n", ##__VA_ARGS__)
    #define UAVCAN_ERROR_PRINT(fmt, ...) printf("[UAVCAN ERROR] " fmt "\n", ##__VA_ARGS__)
#else
    #define UAVCAN_DEBUG_PRINT(fmt, ...)
    #define UAVCAN_INFO_PRINT(fmt, ...)
    #define UAVCAN_WARN_PRINT(fmt, ...)
    #define UAVCAN_ERROR_PRINT(fmt, ...)
#endif

// Assert Macro for Development
#ifdef UAVCAN_DEBUG_ENABLED
    #include <assert.h>
    #define UAVCAN_ASSERT(condition) assert(condition)
#else
    #define UAVCAN_ASSERT(condition)
#endif

// Compiler Attributes
#if defined(__GNUC__)
    #define UAVCAN_PACKED       __attribute__((packed))
    #define UAVCAN_ALIGNED(n)   __attribute__((aligned(n)))
    #define UAVCAN_UNUSED       __attribute__((unused))
#else
    #define UAVCAN_PACKED
    #define UAVCAN_ALIGNED(n)
    #define UAVCAN_UNUSED
#endif

// Thread Safety Macros (to be implemented with actual RTOS primitives)
#define UAVCAN_ENTER_CRITICAL()     // To be implemented with FreeRTOS
#define UAVCAN_EXIT_CRITICAL()      // To be implemented with FreeRTOS

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_COMMON_H
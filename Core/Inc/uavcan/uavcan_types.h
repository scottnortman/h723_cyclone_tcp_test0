/**
 * @file uavcan_types.h
 * @brief Core UAVCAN data structures and type definitions
 * 
 * This file defines the fundamental data types and structures used throughout
 * the UAVCAN implementation for the STM32H723 platform.
 */

#ifndef UAVCAN_TYPES_H
#define UAVCAN_TYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "cmsis_os.h"
#include "core/net.h"

// Forward declare udpard types to avoid circular dependencies
typedef uint8_t UdpardPriority;
typedef uint16_t UdpardNodeID;
typedef uint16_t UdpardPortID;
typedef uint64_t UdpardTransferID;

// Include udpard after forward declarations
#include "udpard.h"

/* Exported constants --------------------------------------------------------*/
#define UAVCAN_NODE_ID_UNSET                0
#define UAVCAN_NODE_ID_MIN                  1
#define UAVCAN_NODE_ID_MAX                  127
#define UAVCAN_UDP_PORT                     9382
#define UAVCAN_SUBJECT_MULTICAST_BASE       0xEF000000UL
#define UAVCAN_SERVICE_MULTICAST_BASE       0xEF010000UL

#define UAVCAN_HEARTBEAT_SUBJECT_ID         7509
#define UAVCAN_DEFAULT_HEARTBEAT_INTERVAL   1000  /* 1 Hz in milliseconds */

#define UAVCAN_MAX_MESSAGE_SIZE             1024
#define UAVCAN_MAX_SUBSCRIPTIONS            16
#define UAVCAN_MESSAGE_QUEUE_SIZE           32

/* Exported types ------------------------------------------------------------*/

/**
 * @brief UAVCAN error codes
 */
typedef enum {
    UAVCAN_ERROR_NONE = 0,
    UAVCAN_ERROR_INIT_FAILED,
    UAVCAN_ERROR_SOCKET_ERROR,
    UAVCAN_ERROR_MEMORY_ERROR,
    UAVCAN_ERROR_INVALID_PARAM,
    UAVCAN_ERROR_TIMEOUT,
    UAVCAN_ERROR_PROTOCOL_ERROR,
    UAVCAN_ERROR_NODE_NOT_INITIALIZED,
    UAVCAN_ERROR_ALREADY_INITIALIZED,
    UAVCAN_ERROR_NETWORK_ERROR
} UavcanError;

/**
 * @brief UAVCAN node states
 */
typedef enum {
    UAVCAN_NODE_STATE_UNINITIALIZED = 0,
    UAVCAN_NODE_STATE_INITIALIZING,
    UAVCAN_NODE_STATE_OPERATIONAL,
    UAVCAN_NODE_STATE_ERROR,
    UAVCAN_NODE_STATE_OFFLINE
} UavcanNodeState;

/**
 * @brief UAVCAN node health states (from standard)
 */
typedef enum {
    UAVCAN_NODE_HEALTH_NOMINAL = 0,
    UAVCAN_NODE_HEALTH_ADVISORY = 1,
    UAVCAN_NODE_HEALTH_CAUTION = 2,
    UAVCAN_NODE_HEALTH_WARNING = 3
} UavcanNodeHealth;

/**
 * @brief UAVCAN node modes (from standard)
 */
typedef enum {
    UAVCAN_NODE_MODE_OPERATIONAL = 0,
    UAVCAN_NODE_MODE_INITIALIZATION = 1,
    UAVCAN_NODE_MODE_MAINTENANCE = 2,
    UAVCAN_NODE_MODE_SOFTWARE_UPDATE = 3,
    UAVCAN_NODE_MODE_OFFLINE = 7
} UavcanNodeMode;

/**
 * @brief UAVCAN message structure
 */
typedef struct {
    struct {
        UdpardPriority priority;
        UdpardNodeID source_node_id;
        UdpardNodeID destination_node_id;
        UdpardPortID subject_id;
        UdpardTransferID transfer_id;
    } header;
    
    struct {
        size_t size;
        void* data;
    } payload;
    
    struct {
        systime_t timestamp;
        uint32_t crc;
    } metadata;
} UavcanMessageFrame;

/**
 * @brief UAVCAN node status information
 */
typedef struct {
    UavcanNodeState state;
    uint16_t node_id;
    UavcanNodeHealth health;
    UavcanNodeMode mode;
    uint32_t uptime_sec;
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t errors_count;
    systime_t last_heartbeat_time;
} UavcanNodeStatus;

/**
 * @brief UAVCAN configuration parameters
 */
typedef struct {
    uint16_t node_id;
    uint16_t udp_port;
    uint32_t multicast_base;
    systime_t heartbeat_interval_ms;
    bool debug_enabled;
    bool auto_start;
} UavcanConfig;

/**
 * @brief UAVCAN error information
 */
typedef struct {
    UavcanError code;
    const char* description;
    systime_t timestamp;
    const char* function;
    int line;
} UavcanErrorInfo;

/**
 * @brief Forward declarations for main structures
 */
typedef struct UavcanNode UavcanNode;
typedef struct UavcanTransport UavcanTransport;
typedef struct UavcanHeartbeat UavcanHeartbeat;

/* Exported macros -----------------------------------------------------------*/

/**
 * @brief Macro for error logging with context
 */
#define UAVCAN_LOG_ERROR(error_code, description) \
    do { \
        UavcanErrorInfo error_info = { \
            .code = (error_code), \
            .description = (description), \
            .timestamp = osKernelSysTick(), \
            .function = __FUNCTION__, \
            .line = __LINE__ \
        }; \
        /* TODO: Add actual logging implementation */ \
    } while(0)

/**
 * @brief Macro to check if node ID is valid
 */
#define UAVCAN_IS_VALID_NODE_ID(id) \
    ((id) >= UAVCAN_NODE_ID_MIN && (id) <= UAVCAN_NODE_ID_MAX)

/**
 * @brief Macro to calculate subject multicast address
 */
#define UAVCAN_SUBJECT_MULTICAST_ADDR(subject_id) \
    (UAVCAN_SUBJECT_MULTICAST_BASE | ((uint32_t)(subject_id)))

/**
 * @brief Macro to calculate service multicast address
 */
#define UAVCAN_SERVICE_MULTICAST_ADDR(node_id) \
    (UAVCAN_SERVICE_MULTICAST_BASE | ((uint32_t)(node_id)))

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_TYPES_H */
#ifndef UAVCAN_TYPES_H
#define UAVCAN_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// UAVCAN/Cyphal Priority Levels (8 levels as per Cyphal specification section 4.1.1.3)
#define CYPHAL_PRIORITY_LEVELS 8
#define CYPHAL_PRIORITY_EXCEPTIONAL 0  // Highest priority
#define CYPHAL_PRIORITY_IMMEDIATE    1
#define CYPHAL_PRIORITY_FAST         2
#define CYPHAL_PRIORITY_HIGH         3
#define CYPHAL_PRIORITY_NOMINAL      4  // Default priority
#define CYPHAL_PRIORITY_LOW          5
#define CYPHAL_PRIORITY_SLOW         6
#define CYPHAL_PRIORITY_OPTIONAL     7  // Lowest priority

// UAVCAN Constants
#define UAVCAN_NODE_ID_UNSET         0
#define UAVCAN_NODE_ID_MIN           1
#define UAVCAN_NODE_ID_MAX           127
#define UAVCAN_SUBJECT_ID_MAX        8191
#define UAVCAN_SERVICE_ID_MAX        511
#define UAVCAN_MAX_PAYLOAD_SIZE      1024
#define UAVCAN_HEARTBEAT_INTERVAL_MS 1000
#define UAVCAN_UDP_PORT_DEFAULT      9382
#define UAVCAN_MULTICAST_ADDR        "239.65.65.65"

// Node Health Status (as per UAVCAN specification)
typedef enum {
    UAVCAN_NODE_HEALTH_NOMINAL = 0,    // Normal operation
    UAVCAN_NODE_HEALTH_ADVISORY = 1,   // Minor issues, still operational
    UAVCAN_NODE_HEALTH_CAUTION = 2,    // Major issues, degraded operation
    UAVCAN_NODE_HEALTH_WARNING = 3     // Critical issues, may fail soon
} UavcanNodeHealth;

// Node Mode (as per UAVCAN specification)
typedef enum {
    UAVCAN_NODE_MODE_OPERATIONAL = 0,  // Normal operation
    UAVCAN_NODE_MODE_INITIALIZATION = 1, // Starting up
    UAVCAN_NODE_MODE_MAINTENANCE = 2,  // Maintenance mode
    UAVCAN_NODE_MODE_SOFTWARE_UPDATE = 3, // Software update in progress
    UAVCAN_NODE_MODE_OFFLINE = 7       // Node is offline
} UavcanNodeMode;

// UAVCAN Error Codes
typedef enum {
    UAVCAN_ERROR_NONE = 0,
    UAVCAN_ERROR_INIT_FAILED,
    UAVCAN_ERROR_NETWORK_UNAVAILABLE,
    UAVCAN_ERROR_SEND_FAILED,
    UAVCAN_ERROR_RECEIVE_FAILED,
    UAVCAN_ERROR_QUEUE_FULL,
    UAVCAN_ERROR_INVALID_CONFIG,
    UAVCAN_ERROR_TIMEOUT,
    UAVCAN_ERROR_INVALID_PARAMETER,
    UAVCAN_ERROR_MEMORY_ALLOCATION,
    UAVCAN_ERROR_NODE_ID_CONFLICT,
    UAVCAN_ERROR_TRANSPORT_ERROR
} UavcanError;

// UAVCAN Message Structure
typedef struct {
    uint32_t subject_id;               // Subject ID for the message
    uint8_t priority;                  // Priority level (0-7)
    size_t payload_size;               // Size of the payload in bytes
    uint8_t* payload;                  // Pointer to payload data
    uint64_t timestamp_usec;           // Timestamp in microseconds
    uint8_t source_node_id;            // Source node ID
    uint8_t destination_node_id;       // Destination node ID (for services)
    bool is_service_request;           // True if this is a service request
    bool is_anonymous;                 // True if anonymous message
} UavcanMessage;

// UAVCAN Node Context
typedef struct {
    uint8_t node_id;                   // Node ID (0 for dynamic allocation)
    bool initialized;                  // Initialization status
    uint32_t uptime_sec;               // Node uptime in seconds
    UavcanNodeHealth health;           // Current node health
    UavcanNodeMode mode;               // Current node mode
    void* dynamic_node_id_allocator;   // Pointer to dynamic node ID allocator
} UavcanNodeContext;

// UAVCAN Configuration Structure
typedef struct {
    uint8_t node_id;                   // Node ID (0 for dynamic allocation)
    uint32_t heartbeat_interval_ms;    // Heartbeat interval in milliseconds
    uint16_t udp_port;                 // UDP port for UAVCAN traffic
    char multicast_addr[16];           // Multicast address string
    bool monitor_enabled;              // Message monitoring flag
    uint8_t log_level;                 // Logging verbosity level
} UavcanConfig;

// UAVCAN Statistics Structure
typedef struct {
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t send_errors;
    uint32_t receive_errors;
    uint32_t queue_overflows;
    uint64_t last_heartbeat_time;
    uint32_t discovered_nodes_count;
    uint32_t messages_sent_by_priority[CYPHAL_PRIORITY_LEVELS];
    uint32_t messages_received_by_priority[CYPHAL_PRIORITY_LEVELS];
    uint32_t queue_overflows_by_priority[CYPHAL_PRIORITY_LEVELS];
} UavcanStatistics;

// UAVCAN Node Information
typedef struct {
    uint8_t node_id;
    UavcanNodeHealth health;
    UavcanNodeMode mode;
    uint64_t last_seen_time;
    uint32_t uptime_sec;
    char name[64];
} UavcanNodeInfo;

// Priority Queue Structure
typedef struct {
    void* priority_queues[CYPHAL_PRIORITY_LEVELS];  // FreeRTOS queue handles
    void* queue_mutex;                              // Mutex for thread safety
    uint32_t queue_depths[CYPHAL_PRIORITY_LEVELS];
    uint32_t overflow_counts[CYPHAL_PRIORITY_LEVELS];
} UavcanPriorityQueue;

// UDP Transport Structure
typedef struct {
    void* udp_socket;                  // CycloneTCP socket handle
    void* multicast_addr;              // IP address structure
    uint16_t port;                     // UDP port
    void* socket_mutex;                // Mutex for thread safety
    void* udpard_instance;             // libudpard instance
} UavcanUdpTransport;

// Heartbeat Service Structure
typedef struct {
    uint32_t interval_ms;              // Heartbeat interval
    bool enabled;                      // Service enabled flag
    void* task_handle;                 // FreeRTOS task handle
    UavcanNodeContext* node_ctx;       // Reference to node context
} UavcanHeartbeatService;

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_TYPES_H
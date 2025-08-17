#include "uavcan/uavcan_common.h"

/**
 * @brief Convert UAVCAN error code to string representation
 * @param error The error code to convert
 * @return String representation of the error
 */
const char* uavcanErrorToString(UavcanError error) {
    switch (error) {
        case UAVCAN_ERROR_NONE:
            return "No Error";
        case UAVCAN_ERROR_INIT_FAILED:
            return "Initialization Failed";
        case UAVCAN_ERROR_NETWORK_UNAVAILABLE:
            return "Network Unavailable";
        case UAVCAN_ERROR_SEND_FAILED:
            return "Send Failed";
        case UAVCAN_ERROR_RECEIVE_FAILED:
            return "Receive Failed";
        case UAVCAN_ERROR_QUEUE_FULL:
            return "Queue Full";
        case UAVCAN_ERROR_INVALID_CONFIG:
            return "Invalid Configuration";
        case UAVCAN_ERROR_TIMEOUT:
            return "Timeout";
        case UAVCAN_ERROR_INVALID_PARAMETER:
            return "Invalid Parameter";
        case UAVCAN_ERROR_MEMORY_ALLOCATION:
            return "Memory Allocation Failed";
        case UAVCAN_ERROR_NODE_ID_CONFLICT:
            return "Node ID Conflict";
        case UAVCAN_ERROR_TRANSPORT_ERROR:
            return "Transport Error";
        default:
            return "Unknown Error";
    }
}

/**
 * @brief Convert UAVCAN node health to string representation
 * @param health The node health status to convert
 * @return String representation of the health status
 */
const char* uavcanNodeHealthToString(UavcanNodeHealth health) {
    switch (health) {
        case UAVCAN_NODE_HEALTH_NOMINAL:
            return "Nominal";
        case UAVCAN_NODE_HEALTH_ADVISORY:
            return "Advisory";
        case UAVCAN_NODE_HEALTH_CAUTION:
            return "Caution";
        case UAVCAN_NODE_HEALTH_WARNING:
            return "Warning";
        default:
            return "Unknown";
    }
}

/**
 * @brief Convert UAVCAN node mode to string representation
 * @param mode The node mode to convert
 * @return String representation of the node mode
 */
const char* uavcanNodeModeToString(UavcanNodeMode mode) {
    switch (mode) {
        case UAVCAN_NODE_MODE_OPERATIONAL:
            return "Operational";
        case UAVCAN_NODE_MODE_INITIALIZATION:
            return "Initialization";
        case UAVCAN_NODE_MODE_MAINTENANCE:
            return "Maintenance";
        case UAVCAN_NODE_MODE_SOFTWARE_UPDATE:
            return "Software Update";
        case UAVCAN_NODE_MODE_OFFLINE:
            return "Offline";
        default:
            return "Unknown";
    }
}

/**
 * @brief Convert UAVCAN priority level to string representation
 * @param priority The priority level to convert (0-7)
 * @return String representation of the priority level
 */
const char* uavcanPriorityToString(uint8_t priority) {
    switch (priority) {
        case CYPHAL_PRIORITY_EXCEPTIONAL:
            return "Exceptional";
        case CYPHAL_PRIORITY_IMMEDIATE:
            return "Immediate";
        case CYPHAL_PRIORITY_FAST:
            return "Fast";
        case CYPHAL_PRIORITY_HIGH:
            return "High";
        case CYPHAL_PRIORITY_NOMINAL:
            return "Nominal";
        case CYPHAL_PRIORITY_LOW:
            return "Low";
        case CYPHAL_PRIORITY_SLOW:
            return "Slow";
        case CYPHAL_PRIORITY_OPTIONAL:
            return "Optional";
        default:
            return "Invalid";
    }
}
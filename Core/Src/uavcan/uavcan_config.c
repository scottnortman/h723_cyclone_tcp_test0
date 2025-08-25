/**
 * @file uavcan_config.c
 * @brief UAVCAN Configuration Management implementation
 */

/* Includes ------------------------------------------------------------------*/
#include "uavcan/uavcan_config.h"
#include "uavcan/uavcan_node.h"

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize UAVCAN configuration with defaults
 * @param config Pointer to configuration structure
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigInit(UavcanConfig* config)
{
    if (config == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Set default configuration values
    config->node_id = UAVCAN_NODE_ID_UNSET;
    config->udp_port = UAVCAN_UDP_PORT;
    config->multicast_base = UAVCAN_SUBJECT_MULTICAST_BASE;
    config->heartbeat_interval_ms = UAVCAN_DEFAULT_HEARTBEAT_INTERVAL;
    config->debug_enabled = false;
    config->auto_start = false;
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Validate UAVCAN configuration parameters
 * @param config Pointer to configuration structure
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigValidate(const UavcanConfig* config)
{
    if (config == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Validate node ID (0 is allowed for dynamic allocation)
    if (config->node_id > UAVCAN_NODE_ID_MAX) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Validate UDP port
    if (config->udp_port == 0) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Validate heartbeat interval (minimum 100ms, maximum 60s)
    if (config->heartbeat_interval_ms < 100 || config->heartbeat_interval_ms > 60000) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Apply configuration to UAVCAN node
 * @param node Pointer to UAVCAN node structure
 * @param config Pointer to configuration structure
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigApply(UavcanNode* node, const UavcanConfig* config)
{
    // TODO: Implement configuration application
    if (node == NULL || config == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Validate configuration first
    UavcanError error = uavcanConfigValidate(config);
    if (error != UAVCAN_ERROR_NONE) {
        return error;
    }
    
    // Copy configuration
    node->config = *config;
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Set node ID in configuration
 * @param config Pointer to configuration structure
 * @param node_id Node ID to set
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetNodeId(UavcanConfig* config, uint16_t node_id)
{
    if (config == NULL || node_id > UAVCAN_NODE_ID_MAX) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    config->node_id = node_id;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Set UDP port in configuration
 * @param config Pointer to configuration structure
 * @param udp_port UDP port to set
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetUdpPort(UavcanConfig* config, uint16_t udp_port)
{
    if (config == NULL || udp_port == 0) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    config->udp_port = udp_port;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Set heartbeat interval in configuration
 * @param config Pointer to configuration structure
 * @param interval_ms Heartbeat interval in milliseconds
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetHeartbeatInterval(UavcanConfig* config, systime_t interval_ms)
{
    if (config == NULL || interval_ms < 100 || interval_ms > 60000) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    config->heartbeat_interval_ms = interval_ms;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Enable/disable debug mode
 * @param config Pointer to configuration structure
 * @param enabled Debug mode enabled flag
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetDebugEnabled(UavcanConfig* config, bool enabled)
{
    if (config == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    config->debug_enabled = enabled;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Enable/disable auto start
 * @param config Pointer to configuration structure
 * @param enabled Auto start enabled flag
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetAutoStart(UavcanConfig* config, bool enabled)
{
    if (config == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    config->auto_start = enabled;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Get current configuration from node
 * @param node Pointer to UAVCAN node structure
 * @param config Pointer to configuration structure to fill
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigGet(const UavcanNode* node, UavcanConfig* config)
{
    if (node == NULL || config == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    *config = node->config;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Print configuration to string
 * @param config Pointer to configuration structure
 * @param buffer Pointer to output buffer
 * @param buffer_size Size of output buffer
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigPrint(const UavcanConfig* config, char* buffer, size_t buffer_size)
{
    if (config == NULL || buffer == NULL || buffer_size == 0) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // TODO: Implement proper configuration printing
    // For now, just a basic implementation
    int written = snprintf(buffer, buffer_size,
        "UAVCAN Configuration:\r\n"
        "  Node ID: %u\r\n"
        "  UDP Port: %u\r\n"
        "  Heartbeat Interval: %lu ms\r\n"
        "  Debug: %s\r\n"
        "  Auto Start: %s\r\n",
        config->node_id,
        config->udp_port,
        config->heartbeat_interval_ms,
        config->debug_enabled ? "Enabled" : "Disabled",
        config->auto_start ? "Enabled" : "Disabled");
    
    if (written < 0 || (size_t)written >= buffer_size) {
        return UAVCAN_ERROR_MEMORY_ERROR;
    }
    
    return UAVCAN_ERROR_NONE;
}
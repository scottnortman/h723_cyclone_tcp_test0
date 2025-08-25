/**
 * @file uavcan_config.h
 * @brief UAVCAN Configuration Management interface
 * 
 * This file defines the interface for UAVCAN configuration management
 * including parameter validation and runtime updates.
 */

#ifndef UAVCAN_CONFIG_H
#define UAVCAN_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Initialize UAVCAN configuration with defaults
 * @param config Pointer to configuration structure
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigInit(UavcanConfig* config);

/**
 * @brief Validate UAVCAN configuration parameters
 * @param config Pointer to configuration structure
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigValidate(const UavcanConfig* config);

/**
 * @brief Apply configuration to UAVCAN node
 * @param node Pointer to UAVCAN node structure
 * @param config Pointer to configuration structure
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigApply(UavcanNode* node, const UavcanConfig* config);

/**
 * @brief Set node ID in configuration
 * @param config Pointer to configuration structure
 * @param node_id Node ID to set
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetNodeId(UavcanConfig* config, uint16_t node_id);

/**
 * @brief Set UDP port in configuration
 * @param config Pointer to configuration structure
 * @param udp_port UDP port to set
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetUdpPort(UavcanConfig* config, uint16_t udp_port);

/**
 * @brief Set heartbeat interval in configuration
 * @param config Pointer to configuration structure
 * @param interval_ms Heartbeat interval in milliseconds
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetHeartbeatInterval(UavcanConfig* config, systime_t interval_ms);

/**
 * @brief Enable/disable debug mode
 * @param config Pointer to configuration structure
 * @param enabled Debug mode enabled flag
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetDebugEnabled(UavcanConfig* config, bool enabled);

/**
 * @brief Enable/disable auto start
 * @param config Pointer to configuration structure
 * @param enabled Auto start enabled flag
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigSetAutoStart(UavcanConfig* config, bool enabled);

/**
 * @brief Get current configuration from node
 * @param node Pointer to UAVCAN node structure
 * @param config Pointer to configuration structure to fill
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigGet(const UavcanNode* node, UavcanConfig* config);

/**
 * @brief Print configuration to string
 * @param config Pointer to configuration structure
 * @param buffer Pointer to output buffer
 * @param buffer_size Size of output buffer
 * @retval UavcanError Error code
 */
UavcanError uavcanConfigPrint(const UavcanConfig* config, char* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_CONFIG_H */
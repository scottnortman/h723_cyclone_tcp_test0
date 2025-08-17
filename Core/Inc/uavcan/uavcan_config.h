#ifndef UAVCAN_CONFIG_H
#define UAVCAN_CONFIG_H

#include "uavcan_types.h"
#include "FreeRTOS.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

// Configuration parameter types
typedef enum {
    UAVCAN_CONFIG_NODE_ID,
    UAVCAN_CONFIG_HEARTBEAT_INTERVAL,
    UAVCAN_CONFIG_UDP_PORT,
    UAVCAN_CONFIG_MULTICAST_ADDR,
    UAVCAN_CONFIG_MONITOR_ENABLED,
    UAVCAN_CONFIG_LOG_LEVEL,
    UAVCAN_CONFIG_MAX_PARAMS
} UavcanConfigParam;

// Configuration value union
typedef union {
    uint8_t uint8_val;
    uint16_t uint16_val;
    uint32_t uint32_val;
    bool bool_val;
    char string_val[32];
} UavcanConfigValue;

// Configuration entry
typedef struct {
    UavcanConfigParam param;
    UavcanConfigValue value;
    bool is_set;
    bool is_valid;
} UavcanConfigEntry;

// Configuration context
typedef struct {
    UavcanConfigEntry entries[UAVCAN_CONFIG_MAX_PARAMS];
    SemaphoreHandle_t mutex;
    bool initialized;
} UavcanConfigContext;

/**
 * @brief Initialize UAVCAN configuration system
 * @param config Pointer to configuration context
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanConfigInit(UavcanConfigContext* config);

/**
 * @brief Set configuration parameter
 * @param config Pointer to configuration context
 * @param param Parameter to set
 * @param value Pointer to value to set
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanConfigSet(UavcanConfigContext* config, UavcanConfigParam param, const UavcanConfigValue* value);

/**
 * @brief Get configuration parameter
 * @param config Pointer to configuration context
 * @param param Parameter to get
 * @param value Pointer to store the value
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanConfigGet(const UavcanConfigContext* config, UavcanConfigParam param, UavcanConfigValue* value);

/**
 * @brief Check if parameter is set
 * @param config Pointer to configuration context
 * @param param Parameter to check
 * @return bool true if set, false otherwise
 */
bool uavcanConfigIsSet(const UavcanConfigContext* config, UavcanConfigParam param);

/**
 * @brief Validate configuration parameter value
 * @param param Parameter to validate
 * @param value Pointer to value to validate
 * @return bool true if valid, false otherwise
 */
bool uavcanConfigValidateParam(UavcanConfigParam param, const UavcanConfigValue* value);

/**
 * @brief Reset configuration to defaults
 * @param config Pointer to configuration context
 * @return error_t UAVCAN_ERROR_NONE on success
 */
error_t uavcanConfigReset(UavcanConfigContext* config);

/**
 * @brief Get configuration as formatted string
 * @param config Pointer to configuration context
 * @param buffer Buffer to store configuration string
 * @param buffer_size Size of the buffer
 * @return size_t Number of characters written
 */
size_t uavcanConfigGetString(const UavcanConfigContext* config, char* buffer, size_t buffer_size);

/**
 * @brief Get parameter name as string
 * @param param Parameter to get name for
 * @return const char* Parameter name string
 */
const char* uavcanConfigGetParamName(UavcanConfigParam param);

/**
 * @brief Parse parameter name from string
 * @param param_name Parameter name string
 * @param param Pointer to store parsed parameter
 * @return bool true if parsed successfully, false otherwise
 */
bool uavcanConfigParseParamName(const char* param_name, UavcanConfigParam* param);

// Helper functions for specific parameter types
error_t uavcanConfigSetNodeId(UavcanConfigContext* config, uint8_t node_id);
error_t uavcanConfigGetNodeId(const UavcanConfigContext* config, uint8_t* node_id);

error_t uavcanConfigSetHeartbeatInterval(UavcanConfigContext* config, uint32_t interval_ms);
error_t uavcanConfigGetHeartbeatInterval(const UavcanConfigContext* config, uint32_t* interval_ms);

error_t uavcanConfigSetUdpPort(UavcanConfigContext* config, uint16_t port);
error_t uavcanConfigGetUdpPort(const UavcanConfigContext* config, uint16_t* port);

error_t uavcanConfigSetMonitorEnabled(UavcanConfigContext* config, bool enabled);
error_t uavcanConfigGetMonitorEnabled(const UavcanConfigContext* config, bool* enabled);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_CONFIG_H
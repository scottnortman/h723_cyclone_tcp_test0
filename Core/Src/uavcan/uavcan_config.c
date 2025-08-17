#include "uavcan/uavcan_config.h"
#include "uavcan/uavcan_common.h"
#include <string.h>
#include <stdio.h>

// Parameter name strings
static const char* param_names[UAVCAN_CONFIG_MAX_PARAMS] = {
    "node-id",
    "heartbeat-interval",
    "udp-port",
    "multicast-addr",
    "monitor-enabled",
    "log-level"
};

// Default values
static const UavcanConfigValue default_values[UAVCAN_CONFIG_MAX_PARAMS] = {
    { .uint8_val = UAVCAN_NODE_ID_UNSET },                    // node-id
    { .uint32_val = UAVCAN_HEARTBEAT_INTERVAL_DEFAULT_MS },   // heartbeat-interval
    { .uint16_val = UAVCAN_UDP_PORT_DEFAULT },                // udp-port
    { .string_val = UAVCAN_MULTICAST_ADDR },                  // multicast-addr
    { .bool_val = false },                                    // monitor-enabled
    { .uint8_val = UAVCAN_LOG_LEVEL_INFO }                    // log-level
};

error_t uavcanConfigInit(UavcanConfigContext* config) {
    if (config == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize configuration context
    memset(config, 0, sizeof(UavcanConfigContext));
    
    // Create mutex for thread safety
    config->mutex = xSemaphoreCreateMutex();
    if (config->mutex == NULL) {
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    // Set default values
    for (int i = 0; i < UAVCAN_CONFIG_MAX_PARAMS; i++) {
        config->entries[i].param = (UavcanConfigParam)i;
        config->entries[i].value = default_values[i];
        config->entries[i].is_set = true;
        config->entries[i].is_valid = true;
    }

    config->initialized = true;
    return UAVCAN_ERROR_NONE;
}

error_t uavcanConfigSet(UavcanConfigContext* config, UavcanConfigParam param, const UavcanConfigValue* value) {
    if (config == NULL || value == NULL || param >= UAVCAN_CONFIG_MAX_PARAMS) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!config->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Validate parameter value
    if (!uavcanConfigValidateParam(param, value)) {
        return UAVCAN_ERROR_INVALID_CONFIG;
    }

    if (xSemaphoreTake(config->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Set the parameter
    config->entries[param].value = *value;
    config->entries[param].is_set = true;
    config->entries[param].is_valid = true;

    xSemaphoreGive(config->mutex);
    return UAVCAN_ERROR_NONE;
}

error_t uavcanConfigGet(const UavcanConfigContext* config, UavcanConfigParam param, UavcanConfigValue* value) {
    if (config == NULL || value == NULL || param >= UAVCAN_CONFIG_MAX_PARAMS) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!config->initialized) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    if (xSemaphoreTake(config->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    if (!config->entries[param].is_set || !config->entries[param].is_valid) {
        xSemaphoreGive(config->mutex);
        return UAVCAN_ERROR_FAILURE;
    }

    *value = config->entries[param].value;

    xSemaphoreGive(config->mutex);
    return UAVCAN_ERROR_NONE;
}

bool uavcanConfigIsSet(const UavcanConfigContext* config, UavcanConfigParam param) {
    if (config == NULL || param >= UAVCAN_CONFIG_MAX_PARAMS || !config->initialized) {
        return false;
    }

    if (xSemaphoreTake(config->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    bool is_set = config->entries[param].is_set && config->entries[param].is_valid;

    xSemaphoreGive(config->mutex);
    return is_set;
}

bool uavcanConfigValidateParam(UavcanConfigParam param, const UavcanConfigValue* value) {
    if (value == NULL) {
        return false;
    }

    switch (param) {
        case UAVCAN_CONFIG_NODE_ID:
            return (value->uint8_val == UAVCAN_NODE_ID_UNSET || 
                   uavcanIsValidNodeId(value->uint8_val));

        case UAVCAN_CONFIG_HEARTBEAT_INTERVAL:
            return (value->uint32_val >= UAVCAN_HEARTBEAT_INTERVAL_MIN_MS && 
                   value->uint32_val <= UAVCAN_HEARTBEAT_INTERVAL_MAX_MS);

        case UAVCAN_CONFIG_UDP_PORT:
            return (value->uint16_val > 0 && value->uint16_val < 65536);

        case UAVCAN_CONFIG_MULTICAST_ADDR:
            // Simple validation - check if string is not empty and reasonable length
            return (strlen(value->string_val) > 0 && strlen(value->string_val) < 32);

        case UAVCAN_CONFIG_MONITOR_ENABLED:
            // Boolean values are always valid
            return true;

        case UAVCAN_CONFIG_LOG_LEVEL:
            return (value->uint8_val <= UAVCAN_LOG_LEVEL_TRACE);

        default:
            return false;
    }
}

error_t uavcanConfigReset(UavcanConfigContext* config) {
    if (config == NULL || !config->initialized) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (xSemaphoreTake(config->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Reset to default values
    for (int i = 0; i < UAVCAN_CONFIG_MAX_PARAMS; i++) {
        config->entries[i].param = (UavcanConfigParam)i;
        config->entries[i].value = default_values[i];
        config->entries[i].is_set = true;
        config->entries[i].is_valid = true;
    }

    xSemaphoreGive(config->mutex);
    return UAVCAN_ERROR_NONE;
}

size_t uavcanConfigGetString(const UavcanConfigContext* config, char* buffer, size_t buffer_size) {
    if (config == NULL || buffer == NULL || buffer_size == 0) {
        return 0;
    }

    if (!config->initialized) {
        return snprintf(buffer, buffer_size, "Configuration not initialized\r\n");
    }

    if (xSemaphoreTake(config->mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return snprintf(buffer, buffer_size, "Configuration unavailable (mutex timeout)\r\n");
    }

    size_t written = snprintf(buffer, buffer_size, "UAVCAN Configuration:\r\n");

    for (int i = 0; i < UAVCAN_CONFIG_MAX_PARAMS && written < buffer_size - 100; i++) {
        const UavcanConfigEntry* entry = &config->entries[i];
        char temp_buffer[100];

        if (!entry->is_set || !entry->is_valid) {
            snprintf(temp_buffer, sizeof(temp_buffer), "  %-18s: <not set>\r\n", param_names[i]);
        } else {
            switch (entry->param) {
                case UAVCAN_CONFIG_NODE_ID:
                    snprintf(temp_buffer, sizeof(temp_buffer), "  %-18s: %d\r\n", 
                            param_names[i], entry->value.uint8_val);
                    break;

                case UAVCAN_CONFIG_HEARTBEAT_INTERVAL:
                    snprintf(temp_buffer, sizeof(temp_buffer), "  %-18s: %lu ms\r\n", 
                            param_names[i], (unsigned long)entry->value.uint32_val);
                    break;

                case UAVCAN_CONFIG_UDP_PORT:
                    snprintf(temp_buffer, sizeof(temp_buffer), "  %-18s: %d\r\n", 
                            param_names[i], entry->value.uint16_val);
                    break;

                case UAVCAN_CONFIG_MULTICAST_ADDR:
                    snprintf(temp_buffer, sizeof(temp_buffer), "  %-18s: %s\r\n", 
                            param_names[i], entry->value.string_val);
                    break;

                case UAVCAN_CONFIG_MONITOR_ENABLED:
                    snprintf(temp_buffer, sizeof(temp_buffer), "  %-18s: %s\r\n", 
                            param_names[i], entry->value.bool_val ? "Yes" : "No");
                    break;

                case UAVCAN_CONFIG_LOG_LEVEL:
                    snprintf(temp_buffer, sizeof(temp_buffer), "  %-18s: %d\r\n", 
                            param_names[i], entry->value.uint8_val);
                    break;

                default:
                    snprintf(temp_buffer, sizeof(temp_buffer), "  %-18s: <unknown>\r\n", param_names[i]);
                    break;
            }
        }

        if (written + strlen(temp_buffer) < buffer_size) {
            strcat(buffer, temp_buffer);
            written += strlen(temp_buffer);
        }
    }

    xSemaphoreGive(config->mutex);
    return written;
}

const char* uavcanConfigGetParamName(UavcanConfigParam param) {
    if (param >= UAVCAN_CONFIG_MAX_PARAMS) {
        return "unknown";
    }
    return param_names[param];
}

bool uavcanConfigParseParamName(const char* param_name, UavcanConfigParam* param) {
    if (param_name == NULL || param == NULL) {
        return false;
    }

    for (int i = 0; i < UAVCAN_CONFIG_MAX_PARAMS; i++) {
        if (strcmp(param_name, param_names[i]) == 0) {
            *param = (UavcanConfigParam)i;
            return true;
        }
    }

    return false;
}

// Helper functions for specific parameter types
error_t uavcanConfigSetNodeId(UavcanConfigContext* config, uint8_t node_id) {
    UavcanConfigValue value = { .uint8_val = node_id };
    return uavcanConfigSet(config, UAVCAN_CONFIG_NODE_ID, &value);
}

error_t uavcanConfigGetNodeId(const UavcanConfigContext* config, uint8_t* node_id) {
    if (node_id == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    UavcanConfigValue value;
    error_t result = uavcanConfigGet(config, UAVCAN_CONFIG_NODE_ID, &value);
    if (result == UAVCAN_ERROR_NONE) {
        *node_id = value.uint8_val;
    }
    return result;
}

error_t uavcanConfigSetHeartbeatInterval(UavcanConfigContext* config, uint32_t interval_ms) {
    UavcanConfigValue value = { .uint32_val = interval_ms };
    return uavcanConfigSet(config, UAVCAN_CONFIG_HEARTBEAT_INTERVAL, &value);
}

error_t uavcanConfigGetHeartbeatInterval(const UavcanConfigContext* config, uint32_t* interval_ms) {
    if (interval_ms == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    UavcanConfigValue value;
    error_t result = uavcanConfigGet(config, UAVCAN_CONFIG_HEARTBEAT_INTERVAL, &value);
    if (result == UAVCAN_ERROR_NONE) {
        *interval_ms = value.uint32_val;
    }
    return result;
}

error_t uavcanConfigSetUdpPort(UavcanConfigContext* config, uint16_t port) {
    UavcanConfigValue value = { .uint16_val = port };
    return uavcanConfigSet(config, UAVCAN_CONFIG_UDP_PORT, &value);
}

error_t uavcanConfigGetUdpPort(const UavcanConfigContext* config, uint16_t* port) {
    if (port == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    UavcanConfigValue value;
    error_t result = uavcanConfigGet(config, UAVCAN_CONFIG_UDP_PORT, &value);
    if (result == UAVCAN_ERROR_NONE) {
        *port = value.uint16_val;
    }
    return result;
}

error_t uavcanConfigSetMonitorEnabled(UavcanConfigContext* config, bool enabled) {
    UavcanConfigValue value = { .bool_val = enabled };
    return uavcanConfigSet(config, UAVCAN_CONFIG_MONITOR_ENABLED, &value);
}

error_t uavcanConfigGetMonitorEnabled(const UavcanConfigContext* config, bool* enabled) {
    if (enabled == NULL) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    UavcanConfigValue value;
    error_t result = uavcanConfigGet(config, UAVCAN_CONFIG_MONITOR_ENABLED, &value);
    if (result == UAVCAN_ERROR_NONE) {
        *enabled = value.bool_val;
    }
    return result;
}
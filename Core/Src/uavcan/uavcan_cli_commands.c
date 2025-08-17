#include "uavcan/uavcan_cli_commands.h"
#include "uavcan/uavcan_priority_queue.h"
#include "uavcan/uavcan_common.h"
#include "uavcan/uavcan_monitor.h"
#include "uavcan/uavcan_node_discovery.h"
#include "uavcan/uavcan_config.h"

// Global references to UAVCAN components (set by main application)
static UavcanNodeContext* g_node_ctx = NULL;
static UavcanHeartbeatService* g_heartbeat_service = NULL;
static UavcanMonitorContext* g_monitor_ctx = NULL;
static UavcanNodeDiscoveryContext* g_discovery_ctx = NULL;
static UavcanConfigContext* g_config_ctx = NULL;

// Forward declarations of command functions
static BaseType_t prvUavcanStatusCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);
static BaseType_t prvUavcanConfigCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);
static BaseType_t prvUavcanHeartbeatCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);
static BaseType_t prvUavcanSendTestCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);
static BaseType_t prvUavcanMonitorCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);
static BaseType_t prvUavcanNodesCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);
static BaseType_t prvUavcanShowConfigCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);
static BaseType_t prvUavcanDiagnosticCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);
static BaseType_t prvUavcanLogLevelCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString);

// Command definitions
static const CLI_Command_Definition_t xUavcanStatus = {
    "uavcan-status",
    "\r\nuavcan-status:\r\n Display UAVCAN node status and statistics\r\n",
    prvUavcanStatusCommand,
    0  // No parameters
};

static const CLI_Command_Definition_t xUavcanConfig = {
    "uavcan-config",
    "\r\nuavcan-config <parameter> <value>:\r\n Configure UAVCAN parameters\r\n"
    " Parameters: node-id, heartbeat-interval\r\n"
    " Examples:\r\n"
    "   uavcan-config node-id 42\r\n"
    "   uavcan-config heartbeat-interval 2000\r\n",
    prvUavcanConfigCommand,
    2  // Two parameters: parameter name and value
};

static const CLI_Command_Definition_t xUavcanHeartbeat = {
    "uavcan-heartbeat",
    "\r\nuavcan-heartbeat <action>:\r\n Control heartbeat service\r\n"
    " Actions: start, stop, send, status\r\n"
    " Examples:\r\n"
    "   uavcan-heartbeat start\r\n"
    "   uavcan-heartbeat stop\r\n"
    "   uavcan-heartbeat send\r\n"
    "   uavcan-heartbeat status\r\n",
    prvUavcanHeartbeatCommand,
    1  // One parameter: action
};

static const CLI_Command_Definition_t xUavcanSendTest = {
    "uavcan-send-test",
    "\r\nuavcan-send-test <subject-id> <priority> [data]:\r\n Send a test UAVCAN message\r\n"
    " subject-id: Subject ID (0-8191)\r\n"
    " priority: Priority level (0-7, where 0 is highest)\r\n"
    " data: Optional test data (default: \"test\")\r\n"
    " Examples:\r\n"
    "   uavcan-send-test 1234 4\r\n"
    "   uavcan-send-test 1234 4 \"Hello UAVCAN\"\r\n",
    prvUavcanSendTestCommand,
    -1  // Variable number of parameters (2-3)
};

static const CLI_Command_Definition_t xUavcanMonitor = {
    "uavcan-monitor",
    "\r\nuavcan-monitor <on|off>:\r\n Enable or disable UAVCAN message monitoring\r\n"
    " Actions: on, off, status\r\n"
    " Examples:\r\n"
    "   uavcan-monitor on\r\n"
    "   uavcan-monitor off\r\n"
    "   uavcan-monitor status\r\n",
    prvUavcanMonitorCommand,
    1  // One parameter: action
};

static const CLI_Command_Definition_t xUavcanNodes = {
    "uavcan-nodes",
    "\r\nuavcan-nodes:\r\n List discovered UAVCAN nodes on the network\r\n",
    prvUavcanNodesCommand,
    0  // No parameters
};

static const CLI_Command_Definition_t xUavcanShowConfig = {
    "uavcan-show-config",
    "\r\nuavcan-show-config:\r\n Display current UAVCAN configuration\r\n",
    prvUavcanShowConfigCommand,
    0  // No parameters
};

static const CLI_Command_Definition_t xUavcanDiagnostic = {
    "uavcan-diag",
    "\r\nuavcan-diag <command>:\r\n Run UAVCAN diagnostic commands\r\n"
    " Commands: network, stats, reset-stats, test-priorities\r\n"
    " Examples:\r\n"
    "   uavcan-diag network\r\n"
    "   uavcan-diag stats\r\n"
    "   uavcan-diag reset-stats\r\n"
    "   uavcan-diag test-priorities\r\n",
    prvUavcanDiagnosticCommand,
    1  // One parameter: command
};

static const CLI_Command_Definition_t xUavcanLogLevel = {
    "uavcan-log-level",
    "\r\nuavcan-log-level [level]:\r\n Set or display UAVCAN logging level\r\n"
    " Levels: 0=None, 1=Error, 2=Warning, 3=Info, 4=Debug, 5=Trace\r\n"
    " Examples:\r\n"
    "   uavcan-log-level\r\n"
    "   uavcan-log-level 3\r\n",
    prvUavcanLogLevelCommand,
    -1  // Variable parameters (0-1)
};

void vRegisterUavcanCLICommands(void)
{
    // Register all UAVCAN CLI commands
    FreeRTOS_CLIRegisterCommand(&xUavcanStatus);
    FreeRTOS_CLIRegisterCommand(&xUavcanConfig);
    FreeRTOS_CLIRegisterCommand(&xUavcanHeartbeat);
    FreeRTOS_CLIRegisterCommand(&xUavcanSendTest);
    FreeRTOS_CLIRegisterCommand(&xUavcanMonitor);
    FreeRTOS_CLIRegisterCommand(&xUavcanNodes);
    FreeRTOS_CLIRegisterCommand(&xUavcanShowConfig);
    FreeRTOS_CLIRegisterCommand(&xUavcanDiagnostic);
    FreeRTOS_CLIRegisterCommand(&xUavcanLogLevel);
}

void uavcanCliSetNodeContext(UavcanNodeContext* node_ctx)
{
    g_node_ctx = node_ctx;
}

void uavcanCliSetHeartbeatService(UavcanHeartbeatService* hb_service)
{
    g_heartbeat_service = hb_service;
}

void uavcanCliSetMonitorContext(UavcanMonitorContext* monitor_ctx)
{
    g_monitor_ctx = monitor_ctx;
}

void uavcanCliSetDiscoveryContext(UavcanNodeDiscoveryContext* discovery_ctx)
{
    g_discovery_ctx = discovery_ctx;
}

void uavcanCliSetConfigContext(UavcanConfigContext* config_ctx)
{
    g_config_ctx = config_ctx;
}

static BaseType_t prvUavcanStatusCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    // Remove compile time warnings about unused parameters
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    if (g_node_ctx == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "UAVCAN node not initialized\r\n");
        return pdFALSE;
    }

    // Get node status information
    char status_buffer[512];
    size_t status_len = uavcanNodeGetStatusString(g_node_ctx, status_buffer, sizeof(status_buffer));
    
    if (status_len > 0) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "%s", status_buffer);
    } else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Failed to get node status\r\n");
    }

    // Add heartbeat service status if available
    if (g_heartbeat_service != NULL) {
        char hb_status[256];
        size_t hb_len = uavcanHeartbeatGetStatusString(g_heartbeat_service, hb_status, sizeof(hb_status));
        if (hb_len > 0 && (strlen(pcWriteBuffer) + hb_len + 2) < xWriteBufferLen) {
            strcat(pcWriteBuffer, "\r\n");
            strcat(pcWriteBuffer, hb_status);
        }
    }

    return pdFALSE;
}

static BaseType_t prvUavcanConfigCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    const char* pcParameter1;
    const char* pcParameter2;
    BaseType_t xParameter1StringLength, xParameter2StringLength;

    configASSERT(pcWriteBuffer);

    if (g_node_ctx == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "UAVCAN node not initialized\r\n");
        return pdFALSE;
    }

    // Get first parameter (parameter name)
    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameter1StringLength);
    if (pcParameter1 == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Missing parameter name\r\n");
        return pdFALSE;
    }

    // Get second parameter (value)
    pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameter2StringLength);
    if (pcParameter2 == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Missing parameter value\r\n");
        return pdFALSE;
    }

    // Parse parameter name
    UavcanConfigParam param;
    char param_name[32];
    snprintf(param_name, sizeof(param_name), "%.*s", (int)xParameter1StringLength, pcParameter1);
    
    if (!uavcanConfigParseParamName(param_name, &param)) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Unknown parameter '%s'\r\n", param_name);
        return pdFALSE;
    }

    // Handle node-id configuration
    if (param == UAVCAN_CONFIG_NODE_ID) {
        uint8_t node_id = (uint8_t)atoi(pcParameter2);
        
        if (node_id != UAVCAN_NODE_ID_UNSET && (node_id < UAVCAN_NODE_ID_MIN || node_id > UAVCAN_NODE_ID_MAX)) {
            snprintf(pcWriteBuffer, xWriteBufferLen, 
                    "Error: Invalid node ID %d (valid range: %d-%d or 0 for dynamic)\r\n", 
                    node_id, UAVCAN_NODE_ID_MIN, UAVCAN_NODE_ID_MAX);
            return pdFALSE;
        }

        // Update configuration
        if (g_config_ctx != NULL) {
            error_t config_result = uavcanConfigSetNodeId(g_config_ctx, node_id);
            if (config_result != UAVCAN_ERROR_NONE) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to save node ID configuration\r\n");
                return pdFALSE;
            }
        }

        // Update node context if available
        if (g_node_ctx != NULL) {
            error_t result = uavcanNodeSetId(g_node_ctx, node_id);
            if (result == UAVCAN_ERROR_NONE) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Node ID set to %d\r\n", node_id);
            } else {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to set node ID\r\n");
            }
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Node ID configuration saved (will apply on restart)\r\n");
        }
    }
    // Handle heartbeat interval configuration
    else if (param == UAVCAN_CONFIG_HEARTBEAT_INTERVAL) {
        uint32_t interval_ms = (uint32_t)atoi(pcParameter2);
        
        if (!uavcanHeartbeatValidateInterval(interval_ms)) {
            snprintf(pcWriteBuffer, xWriteBufferLen, 
                    "Error: Invalid heartbeat interval %lu ms (valid range: %d-%d ms)\r\n", 
                    (unsigned long)interval_ms, 
                    UAVCAN_HEARTBEAT_INTERVAL_MIN_MS, 
                    UAVCAN_HEARTBEAT_INTERVAL_MAX_MS);
            return pdFALSE;
        }

        // Update configuration
        if (g_config_ctx != NULL) {
            error_t config_result = uavcanConfigSetHeartbeatInterval(g_config_ctx, interval_ms);
            if (config_result != UAVCAN_ERROR_NONE) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to save heartbeat interval configuration\r\n");
                return pdFALSE;
            }
        }

        // Update heartbeat service if available
        if (g_heartbeat_service != NULL) {
            error_t result = uavcanHeartbeatSetInterval(g_heartbeat_service, interval_ms);
            if (result == UAVCAN_ERROR_NONE) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Heartbeat interval set to %lu ms\r\n", (unsigned long)interval_ms);
            } else {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to set heartbeat interval\r\n");
            }
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Heartbeat interval configuration saved (will apply on restart)\r\n");
        }
    }
    else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Parameter '%s' is not configurable via CLI\r\n", param_name);
    }

    return pdFALSE;
}

static BaseType_t prvUavcanHeartbeatCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    const char* pcParameter;
    BaseType_t xParameterStringLength;

    configASSERT(pcWriteBuffer);

    if (g_heartbeat_service == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Heartbeat service not available\r\n");
        return pdFALSE;
    }

    // Get action parameter
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    if (pcParameter == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Missing action parameter\r\n");
        return pdFALSE;
    }

    // Handle start action
    if (strncmp(pcParameter, "start", xParameterStringLength) == 0) {
        error_t result = uavcanHeartbeatStart(g_heartbeat_service);
        if (result == UAVCAN_ERROR_NONE) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Heartbeat service started\r\n");
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to start heartbeat service\r\n");
        }
    }
    // Handle stop action
    else if (strncmp(pcParameter, "stop", xParameterStringLength) == 0) {
        error_t result = uavcanHeartbeatStop(g_heartbeat_service);
        if (result == UAVCAN_ERROR_NONE) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Heartbeat service stopped\r\n");
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to stop heartbeat service\r\n");
        }
    }
    // Handle send action
    else if (strncmp(pcParameter, "send", xParameterStringLength) == 0) {
        error_t result = uavcanHeartbeatSendNow(g_heartbeat_service);
        if (result == UAVCAN_ERROR_NONE) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Heartbeat message sent\r\n");
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to send heartbeat message\r\n");
        }
    }
    // Handle status action
    else if (strncmp(pcParameter, "status", xParameterStringLength) == 0) {
        char status_buffer[256];
        size_t status_len = uavcanHeartbeatGetStatusString(g_heartbeat_service, status_buffer, sizeof(status_buffer));
        if (status_len > 0) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "%s", status_buffer);
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Failed to get heartbeat status\r\n");
        }
    }
    else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Unknown action '%.*s'\r\n", 
                (int)xParameterStringLength, pcParameter);
    }

    return pdFALSE;
}

static BaseType_t prvUavcanSendTestCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    const char* pcParameter1;
    const char* pcParameter2;
    const char* pcParameter3;
    BaseType_t xParameter1StringLength, xParameter2StringLength, xParameter3StringLength;

    configASSERT(pcWriteBuffer);

    // Get subject ID parameter
    pcParameter1 = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameter1StringLength);
    if (pcParameter1 == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Missing subject ID\r\n");
        return pdFALSE;
    }

    // Get priority parameter
    pcParameter2 = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameter2StringLength);
    if (pcParameter2 == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Missing priority\r\n");
        return pdFALSE;
    }

    // Get optional data parameter
    pcParameter3 = FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParameter3StringLength);

    // Parse subject ID
    uint32_t subject_id = (uint32_t)atoi(pcParameter1);
    if (subject_id > UAVCAN_SUBJECT_ID_MAX) {
        snprintf(pcWriteBuffer, xWriteBufferLen, 
                "Error: Invalid subject ID %lu (max: %d)\r\n", 
                (unsigned long)subject_id, UAVCAN_SUBJECT_ID_MAX);
        return pdFALSE;
    }

    // Parse priority
    uint8_t priority = (uint8_t)atoi(pcParameter2);
    if (priority >= CYPHAL_PRIORITY_LEVELS) {
        snprintf(pcWriteBuffer, xWriteBufferLen, 
                "Error: Invalid priority %d (valid range: 0-%d)\r\n", 
                priority, CYPHAL_PRIORITY_LEVELS - 1);
        return pdFALSE;
    }

    // Prepare test data
    const char* test_data = "test";
    size_t data_len = 4;
    
    if (pcParameter3 != NULL) {
        test_data = pcParameter3;
        data_len = (size_t)xParameter3StringLength;
    }

    // Create and send test message
    UavcanMessage test_msg;
    error_t result = uavcanMessageCreate(&test_msg, subject_id, priority, test_data, data_len);
    
    if (result != UAVCAN_ERROR_NONE) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to create test message\r\n");
        return pdFALSE;
    }

    // Set source node ID if available
    if (g_node_ctx != NULL) {
        test_msg.source_node_id = uavcanNodeGetId(g_node_ctx);
    }

    // TODO: Send message through priority queue system
    // For now, just report success and clean up
    snprintf(pcWriteBuffer, xWriteBufferLen, 
            "Test message created: Subject ID %lu, Priority %d (%s), Data: \"%.*s\"\r\n"
            "Note: Actual transmission requires priority queue integration\r\n",
            (unsigned long)subject_id, priority, uavcanPriorityToString(priority), 
            (int)data_len, test_data);

    // Clean up message
    uavcanMessageDestroy(&test_msg);

    return pdFALSE;
}

static BaseType_t prvUavcanMonitorCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    const char* pcParameter;
    BaseType_t xParameterStringLength;

    configASSERT(pcWriteBuffer);

    // Get action parameter
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    if (pcParameter == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Missing action parameter\r\n");
        return pdFALSE;
    }

    // Handle on action
    if (strncmp(pcParameter, "on", xParameterStringLength) == 0) {
        if (g_monitor_ctx == NULL) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Monitor not initialized\r\n");
        } else {
            error_t result = uavcanMonitorEnable(g_monitor_ctx);
            if (result == UAVCAN_ERROR_NONE) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "UAVCAN message monitoring enabled\r\n");
            } else {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to enable monitoring\r\n");
            }
        }
    }
    // Handle off action
    else if (strncmp(pcParameter, "off", xParameterStringLength) == 0) {
        if (g_monitor_ctx == NULL) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Monitor not initialized\r\n");
        } else {
            error_t result = uavcanMonitorDisable(g_monitor_ctx);
            if (result == UAVCAN_ERROR_NONE) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "UAVCAN message monitoring disabled\r\n");
            } else {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to disable monitoring\r\n");
            }
        }
    }
    // Handle status action
    else if (strncmp(pcParameter, "status", xParameterStringLength) == 0) {
        if (g_monitor_ctx == NULL) {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Monitor not initialized\r\n");
        } else {
            size_t status_len = uavcanMonitorGetStatusString(g_monitor_ctx, pcWriteBuffer, xWriteBufferLen);
            if (status_len == 0) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get monitor status\r\n");
            }
        }
    }
    else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Unknown action '%.*s'\r\n", 
                (int)xParameterStringLength, pcParameter);
    }

    return pdFALSE;
}

static BaseType_t prvUavcanNodesCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    // Remove compile time warnings about unused parameters
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    if (g_discovery_ctx == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Node discovery not initialized\r\n");
        return pdFALSE;
    }

    // Get discovered nodes list
    size_t nodes_len = uavcanNodeDiscoveryGetNodesString(g_discovery_ctx, pcWriteBuffer, xWriteBufferLen);
    
    if (nodes_len == 0) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get nodes list\r\n");
    }

    // Add local node information if available and there's space
    if (g_node_ctx != NULL && uavcanNodeIsInitialized(g_node_ctx) && 
        (strlen(pcWriteBuffer) + 200) < xWriteBufferLen) {
        
        char temp_buffer[200];
        snprintf(temp_buffer, sizeof(temp_buffer),
                "\r\nLocal Node Information:\r\n"
                "  Node ID: %d\r\n"
                "  Health: %s\r\n"
                "  Mode: %s\r\n"
                "  Uptime: %lu seconds\r\n",
                uavcanNodeGetId(g_node_ctx),
                uavcanNodeHealthToString(uavcanNodeGetHealth(g_node_ctx)),
                uavcanNodeModeToString(uavcanNodeGetMode(g_node_ctx)),
                (unsigned long)uavcanNodeGetUptime(g_node_ctx));
        
        strcat(pcWriteBuffer, temp_buffer);
    }

    return pdFALSE;
}s
tatic BaseType_t prvUavcanShowConfigCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    // Remove compile time warnings about unused parameters
    (void)pcCommandString;
    configASSERT(pcWriteBuffer);

    if (g_config_ctx == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Configuration system not initialized\r\n");
        return pdFALSE;
    }

    // Get configuration string
    size_t config_len = uavcanConfigGetString(g_config_ctx, pcWriteBuffer, xWriteBufferLen);
    
    if (config_len == 0) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get configuration\r\n");
    }

    return pdFALSE;
}static 
BaseType_t prvUavcanDiagnosticCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    const char* pcParameter;
    BaseType_t xParameterStringLength;

    configASSERT(pcWriteBuffer);

    // Get command parameter
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    if (pcParameter == NULL) {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Missing diagnostic command\r\n");
        return pdFALSE;
    }

    // Handle network diagnostic
    if (strncmp(pcParameter, "network", xParameterStringLength) == 0) {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                "UAVCAN Network Diagnostics:\r\n"
                "  Transport: UDP/IP\r\n"
                "  Multicast Address: %s\r\n"
                "  UDP Port: %d\r\n"
                "  Node ID: %s\r\n"
                "  Network Status: %s\r\n",
                UAVCAN_MULTICAST_ADDR,
                UAVCAN_UDP_PORT_DEFAULT,
                (g_node_ctx != NULL) ? 
                    (uavcanNodeGetId(g_node_ctx) == UAVCAN_NODE_ID_UNSET ? "Dynamic" : "Static") : "Unknown",
                (g_node_ctx != NULL && uavcanNodeIsInitialized(g_node_ctx)) ? "Active" : "Inactive");
    }
    // Handle statistics diagnostic
    else if (strncmp(pcParameter, "stats", xParameterStringLength) == 0) {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                "UAVCAN Statistics:\r\n"
                "  Messages Sent: 0 (not implemented)\r\n"
                "  Messages Received: 0 (not implemented)\r\n"
                "  Send Errors: 0 (not implemented)\r\n"
                "  Receive Errors: 0 (not implemented)\r\n"
                "  Queue Overflows: 0 (not implemented)\r\n"
                "  Active Nodes: %lu\r\n",
                (g_discovery_ctx != NULL) ? 
                    (unsigned long)uavcanNodeDiscoveryGetActiveCount(g_discovery_ctx) : 0UL);
    }
    // Handle reset statistics
    else if (strncmp(pcParameter, "reset-stats", xParameterStringLength) == 0) {
        // Reset monitor statistics if available
        if (g_monitor_ctx != NULL) {
            error_t result = uavcanMonitorReset(g_monitor_ctx);
            if (result == UAVCAN_ERROR_NONE) {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Monitor statistics reset\r\n");
            } else {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to reset monitor statistics\r\n");
            }
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Statistics reset (monitor not available)\r\n");
        }
    }
    // Handle priority test
    else if (strncmp(pcParameter, "test-priorities", xParameterStringLength) == 0) {
        snprintf(pcWriteBuffer, xWriteBufferLen,
                "UAVCAN Priority Levels Test:\r\n"
                "  0 - %s (Highest)\r\n"
                "  1 - %s\r\n"
                "  2 - %s\r\n"
                "  3 - %s\r\n"
                "  4 - %s (Default)\r\n"
                "  5 - %s\r\n"
                "  6 - %s\r\n"
                "  7 - %s (Lowest)\r\n"
                "Use 'uavcan-send-test' with different priorities to test ordering\r\n",
                uavcanPriorityToString(0), uavcanPriorityToString(1),
                uavcanPriorityToString(2), uavcanPriorityToString(3),
                uavcanPriorityToString(4), uavcanPriorityToString(5),
                uavcanPriorityToString(6), uavcanPriorityToString(7));
    }
    else {
        snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Unknown diagnostic command '%.*s'\r\n", 
                (int)xParameterStringLength, pcParameter);
    }

    return pdFALSE;
}

static BaseType_t prvUavcanLogLevelCommand(char* pcWriteBuffer, size_t xWriteBufferLen, const char* pcCommandString)
{
    const char* pcParameter;
    BaseType_t xParameterStringLength;

    configASSERT(pcWriteBuffer);

    // Get level parameter (optional)
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    
    if (pcParameter == NULL) {
        // Display current log level
        if (g_config_ctx != NULL) {
            UavcanConfigValue value;
            error_t result = uavcanConfigGet(g_config_ctx, UAVCAN_CONFIG_LOG_LEVEL, &value);
            if (result == UAVCAN_ERROR_NONE) {
                const char* level_names[] = {"None", "Error", "Warning", "Info", "Debug", "Trace"};
                const char* level_name = (value.uint8_val <= UAVCAN_LOG_LEVEL_TRACE) ? 
                                        level_names[value.uint8_val] : "Unknown";
                snprintf(pcWriteBuffer, xWriteBufferLen, 
                        "Current log level: %d (%s)\r\n", value.uint8_val, level_name);
            } else {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to get current log level\r\n");
            }
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Configuration system not available\r\n");
        }
    } else {
        // Set new log level
        uint8_t new_level = (uint8_t)atoi(pcParameter);
        
        if (new_level > UAVCAN_LOG_LEVEL_TRACE) {
            snprintf(pcWriteBuffer, xWriteBufferLen, 
                    "Error: Invalid log level %d (valid range: 0-%d)\r\n", 
                    new_level, UAVCAN_LOG_LEVEL_TRACE);
            return pdFALSE;
        }

        if (g_config_ctx != NULL) {
            UavcanConfigValue value = { .uint8_val = new_level };
            error_t result = uavcanConfigSet(g_config_ctx, UAVCAN_CONFIG_LOG_LEVEL, &value);
            if (result == UAVCAN_ERROR_NONE) {
                const char* level_names[] = {"None", "Error", "Warning", "Info", "Debug", "Trace"};
                const char* level_name = level_names[new_level];
                snprintf(pcWriteBuffer, xWriteBufferLen, 
                        "Log level set to %d (%s)\r\n", new_level, level_name);
            } else {
                snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Failed to set log level\r\n");
            }
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen, "Error: Configuration system not available\r\n");
        }
    }

    return pdFALSE;
}
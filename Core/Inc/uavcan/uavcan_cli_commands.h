#ifndef UAVCAN_CLI_COMMANDS_H
#define UAVCAN_CLI_COMMANDS_H

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

/* UAVCAN includes */
#include "uavcan_types.h"
#include "uavcan_heartbeat_service.h"
#include "uavcan_node.h"

// Forward declarations to avoid circular dependency
typedef struct UavcanMonitorContext UavcanMonitorContext;
typedef struct UavcanNodeDiscoveryContext UavcanNodeDiscoveryContext;
typedef struct UavcanConfigContext UavcanConfigContext;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register all UAVCAN CLI commands
 * 
 * This function registers all UAVCAN-related CLI commands with the FreeRTOS+CLI system.
 * It should be called during system initialization after the CLI system is ready.
 */
void vRegisterUavcanCLICommands(void);

/**
 * @brief Set the global UAVCAN node context for CLI commands
 * 
 * @param node_ctx Pointer to the UAVCAN node context
 */
void uavcanCliSetNodeContext(UavcanNodeContext* node_ctx);

/**
 * @brief Set the global UAVCAN heartbeat service for CLI commands
 * 
 * @param hb_service Pointer to the UAVCAN heartbeat service
 */
void uavcanCliSetHeartbeatService(UavcanHeartbeatService* hb_service);

/**
 * @brief Set the global UAVCAN monitor context for CLI commands
 * 
 * @param monitor_ctx Pointer to the UAVCAN monitor context
 */
void uavcanCliSetMonitorContext(UavcanMonitorContext* monitor_ctx);

/**
 * @brief Set the global UAVCAN node discovery context for CLI commands
 * 
 * @param discovery_ctx Pointer to the UAVCAN node discovery context
 */
void uavcanCliSetDiscoveryContext(UavcanNodeDiscoveryContext* discovery_ctx);

/**
 * @brief Set the global UAVCAN configuration context for CLI commands
 * 
 * @param config_ctx Pointer to the UAVCAN configuration context
 */
void uavcanCliSetConfigContext(UavcanConfigContext* config_ctx);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_CLI_COMMANDS_H
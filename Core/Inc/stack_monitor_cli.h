/**
 * @file stack_monitor_cli.h
 * @brief FreeRTOS Stack Monitoring CLI Commands Header
 * 
 * This header provides the interface for comprehensive FreeRTOS stack and
 * memory monitoring CLI commands to help debug stack overflow issues.
 */

#ifndef STACK_MONITOR_CLI_H
#define STACK_MONITOR_CLI_H

#include "FreeRTOS.h"
#include "task.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register all stack monitoring CLI commands
 * 
 * This function registers the following CLI commands:
 * - stack-info: Show stack usage for all tasks
 * - stack-check: Check for stack overflow conditions
 * - stack-watch: Monitor specific task stack usage
 * - heap-info: Show heap usage statistics
 * - memory-info: Show comprehensive memory information
 */
void vRegisterStackMonitorCLICommands(void);

#ifdef __cplusplus
}
#endif

#endif /* STACK_MONITOR_CLI_H */
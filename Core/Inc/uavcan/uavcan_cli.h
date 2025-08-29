/**
 * @file uavcan_cli.h
 * @brief UAVCAN Command Line Interface
 * 
 * This file defines the CLI commands for UAVCAN functionality
 * including testing, configuration, and monitoring.
 */

#ifndef UAVCAN_CLI_H
#define UAVCAN_CLI_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "FreeRTOS_CLI.h"
#include "uavcan_types.h"

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Register all UAVCAN CLI commands
 */
void uavcanCliRegisterCommands(void);

/**
 * @brief UAVCAN test command handler
 * @param pcWriteBuffer Buffer to write response to
 * @param xWriteBufferLen Size of write buffer
 * @param pcCommandString Command string
 * @retval BaseType_t Command result
 */
BaseType_t uavcanCliTestCommand(char* pcWriteBuffer, 
                               size_t xWriteBufferLen, 
                               const char* pcCommandString);

/**
 * @brief UAVCAN system test command handler
 * @param pcWriteBuffer Buffer to write response to
 * @param xWriteBufferLen Size of write buffer
 * @param pcCommandString Command string
 * @retval BaseType_t Command result
 */
BaseType_t uavcanCliSystemTestCommand(char* pcWriteBuffer, 
                                     size_t xWriteBufferLen, 
                                     const char* pcCommandString);

/**
 * @brief UAVCAN status command handler
 * @param pcWriteBuffer Buffer to write response to
 * @param xWriteBufferLen Size of write buffer
 * @param pcCommandString Command string
 * @retval BaseType_t Command result
 */
BaseType_t uavcanCliStatusCommand(char* pcWriteBuffer, 
                                 size_t xWriteBufferLen, 
                                 const char* pcCommandString);

/**
 * @brief UAVCAN requirements verification test command handler
 * @param pcWriteBuffer Buffer to write response to
 * @param xWriteBufferLen Size of write buffer
 * @param pcCommandString Command string
 * @retval BaseType_t Command result
 */
BaseType_t uavcanCliRequirementsTestCommand(char* pcWriteBuffer, 
                                           size_t xWriteBufferLen, 
                                           const char* pcCommandString);

/**
 * @brief UAVCAN simple verify command handler
 * @param pcWriteBuffer Buffer to write response to
 * @param xWriteBufferLen Size of write buffer
 * @param pcCommandString Command string
 * @retval BaseType_t Command result
 */
BaseType_t uavcanCliSimpleVerifyCommand(char* pcWriteBuffer, 
                                       size_t xWriteBufferLen, 
                                       const char* pcCommandString);

/**
 * @brief UAVCAN CLI buffer test command handler
 * @param pcWriteBuffer Buffer to write response to
 * @param xWriteBufferLen Size of write buffer
 * @param pcCommandString Command string
 * @retval BaseType_t Command result
 */
BaseType_t uavcanCliBufferTestCommand(char* pcWriteBuffer, 
                                     size_t xWriteBufferLen, 
                                     const char* pcCommandString);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_CLI_H */
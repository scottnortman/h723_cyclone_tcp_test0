/**
 * @file uavcan.h
 * @brief Main UAVCAN integration header
 * 
 * This file provides the main interface for UAVCAN functionality
 * on the STM32H723 platform with CycloneTCP integration.
 */

#ifndef UAVCAN_H
#define UAVCAN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"
#include "uavcan_node.h"
#include "uavcan_transport.h"
#include "uavcan_messages.h"
#include "uavcan_heartbeat.h"
#include "uavcan_config.h"

/* Exported constants --------------------------------------------------------*/
#define UAVCAN_VERSION_MAJOR    1
#define UAVCAN_VERSION_MINOR    0
#define UAVCAN_VERSION_PATCH    0

/* Exported macros -----------------------------------------------------------*/
#define UAVCAN_VERSION_STRING   "1.0.0"

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Get UAVCAN library version string
 * @retval const char* Version string
 */
const char* uavcanGetVersionString(void);

/**
 * @brief Get UAVCAN library version numbers
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 */
void uavcanGetVersion(uint8_t* major, uint8_t* minor, uint8_t* patch);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_H */
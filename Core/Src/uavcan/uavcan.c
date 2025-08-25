/**
 * @file uavcan.c
 * @brief Main UAVCAN integration implementation
 */

/* Includes ------------------------------------------------------------------*/
#include "uavcan/uavcan.h"

/* Private constants ---------------------------------------------------------*/
static const char* const UAVCAN_VERSION_STR = UAVCAN_VERSION_STRING;

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Get UAVCAN library version string
 * @retval const char* Version string
 */
const char* uavcanGetVersionString(void)
{
    return UAVCAN_VERSION_STR;
}

/**
 * @brief Get UAVCAN library version numbers
 * @param major Pointer to store major version
 * @param minor Pointer to store minor version
 * @param patch Pointer to store patch version
 */
void uavcanGetVersion(uint8_t* major, uint8_t* minor, uint8_t* patch)
{
    if (major != NULL) {
        *major = UAVCAN_VERSION_MAJOR;
    }
    if (minor != NULL) {
        *minor = UAVCAN_VERSION_MINOR;
    }
    if (patch != NULL) {
        *patch = UAVCAN_VERSION_PATCH;
    }
}
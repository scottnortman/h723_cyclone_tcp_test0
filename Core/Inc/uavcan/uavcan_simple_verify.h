/**
 * @file uavcan_simple_verify.h
 * @brief Simple UAVCAN Requirements Verification Interface
 */

#ifndef UAVCAN_SIMPLE_VERIFY_H
#define UAVCAN_SIMPLE_VERIFY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"
#include "core/net.h"

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Simple requirements verification test
 * @param interface Pointer to network interface
 * @retval UavcanError Error code
 */
UavcanError uavcanSimpleVerify(NetInterface* interface);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_SIMPLE_VERIFY_H */
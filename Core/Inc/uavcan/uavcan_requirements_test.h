/**
 * @file uavcan_requirements_test.h
 * @brief UAVCAN Requirements Verification Test Suite Interface
 * 
 * This file defines the interface for formal verification tests that
 * validate all UAVCAN requirements from the specification document.
 */

#ifndef UAVCAN_REQUIREMENTS_TEST_H
#define UAVCAN_REQUIREMENTS_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"
#include "uavcan_test.h"

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Run comprehensive requirements verification tests
 * @param suite Pointer to test suite structure
 * @param interface Pointer to network interface for testing
 * @retval UavcanError Error code
 */
UavcanError uavcanRequirementsVerificationTest(UavcanTestSuite* suite, NetInterface* interface);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_REQUIREMENTS_TEST_H */
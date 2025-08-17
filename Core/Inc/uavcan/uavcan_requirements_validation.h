#ifndef UAVCAN_REQUIREMENTS_VALIDATION_H
#define UAVCAN_REQUIREMENTS_VALIDATION_H

#include "uavcan_integration.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Validate all UAVCAN requirements from the specification
 * 
 * This function validates that the UAVCAN implementation meets all
 * requirements specified in the requirements document:
 * - Requirement 1: Node initialization and configuration
 * - Requirement 2: Message handling and prioritization
 * - Requirement 3: Network monitoring and diagnostics
 * - Requirement 4: Configuration management
 * - Requirement 5: System integration and coexistence
 * - Requirement 6: Heartbeat functionality
 * - Requirement 7: Testing and diagnostics
 * 
 * @param ctx Pointer to initialized UAVCAN integration context
 * @return true if all requirements are met, false otherwise
 */
bool uavcanValidateAllRequirements(UavcanIntegrationContext* ctx);

/**
 * @brief Run complete validation suite including requirements and tests
 * 
 * This function runs the complete validation suite:
 * 1. Requirements validation
 * 2. Comprehensive functional tests
 * 3. Stress testing with high message loads
 * 
 * @param ctx Pointer to initialized UAVCAN integration context
 * @return true if complete validation passes, false otherwise
 */
bool uavcanRunCompleteValidation(UavcanIntegrationContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_REQUIREMENTS_VALIDATION_H
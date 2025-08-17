#ifndef UAVCAN_COMPREHENSIVE_TEST_SUITE_H
#define UAVCAN_COMPREHENSIVE_TEST_SUITE_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Test statistics structure
typedef struct {
    uint32_t tests_run;
    uint32_t tests_passed;
    uint32_t tests_failed;
    uint32_t start_time_ms;
    uint32_t end_time_ms;
} UavcanTestStatistics;

/**
 * @brief Run the complete UAVCAN test suite
 * 
 * This function runs all comprehensive tests including:
 * - Basic functionality tests
 * - Performance and stress tests
 * - Interoperability tests
 * - Message priority handling validation
 * - System stability verification
 * 
 * @return true if all tests pass, false if any test fails
 */
bool uavcanRunComprehensiveTests(void);

/**
 * @brief Get test execution statistics
 * 
 * @return Pointer to test statistics structure
 */
const UavcanTestStatistics* uavcanGetTestStatistics(void);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_COMPREHENSIVE_TEST_SUITE_H
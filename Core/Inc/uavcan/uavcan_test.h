/**
 * @file uavcan_test.h
 * @brief UAVCAN HIL (Hardware-in-the-Loop) Test interface
 * 
 * This file defines the interface for UAVCAN HIL testing functionality
 * to verify node manager and core functionality works as expected.
 */

#ifndef UAVCAN_TEST_H
#define UAVCAN_TEST_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"
#include "uavcan_node.h"

/* Exported constants --------------------------------------------------------*/
#define UAVCAN_TEST_MAX_RESULTS         32
#define UAVCAN_TEST_TIMEOUT_MS          5000

/* Exported types ------------------------------------------------------------*/

/**
 * @brief Test result enumeration
 */
typedef enum {
    UAVCAN_TEST_RESULT_PASS = 0,
    UAVCAN_TEST_RESULT_FAIL,
    UAVCAN_TEST_RESULT_SKIP,
    UAVCAN_TEST_RESULT_TIMEOUT,
    UAVCAN_TEST_RESULT_ERROR
} UavcanTestResult;

/**
 * @brief Individual test case structure
 */
typedef struct {
    const char* name;
    const char* description;
    UavcanTestResult result;
    uint32_t execution_time_ms;
    const char* error_message;
} UavcanTestCase;

/**
 * @brief Test suite structure
 */
typedef struct {
    const char* suite_name;
    UavcanTestCase test_cases[UAVCAN_TEST_MAX_RESULTS];
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    uint32_t skipped_tests;
    uint32_t total_execution_time_ms;
    bool suite_completed;
} UavcanTestSuite;

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Initialize UAVCAN test suite
 * @param suite Pointer to test suite structure
 * @param suite_name Name of the test suite
 * @retval UavcanError Error code
 */
UavcanError uavcanTestInit(UavcanTestSuite* suite, const char* suite_name);

/**
 * @brief Run all UAVCAN node manager tests
 * @param suite Pointer to test suite structure
 * @param interface Pointer to network interface for testing
 * @retval UavcanError Error code
 */
UavcanError uavcanTestRunNodeManagerTests(UavcanTestSuite* suite, NetInterface* interface);

/**
 * @brief Test node initialization and deinitialization
 * @param suite Pointer to test suite structure
 * @param interface Pointer to network interface
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestNodeInitDeinit(UavcanTestSuite* suite, NetInterface* interface);

/**
 * @brief Test node ID management
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestNodeIdManagement(UavcanTestSuite* suite, UavcanNode* node);

/**
 * @brief Test node state management
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestNodeStateManagement(UavcanTestSuite* suite, UavcanNode* node);

/**
 * @brief Test node health and mode management
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestNodeHealthMode(UavcanTestSuite* suite, UavcanNode* node);

/**
 * @brief Test dynamic node ID allocation
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestDynamicNodeId(UavcanTestSuite* suite, UavcanNode* node);

/**
 * @brief Test memory management
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestMemoryManagement(UavcanTestSuite* suite, UavcanNode* node);

/**
 * @brief Test transport integration
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestTransportIntegration(UavcanTestSuite* suite, UavcanNode* node);

/**
 * @brief Print test suite results
 * @param suite Pointer to test suite structure
 */
void uavcanTestPrintResults(const UavcanTestSuite* suite);

/**
 * @brief Add test case result to suite
 * @param suite Pointer to test suite structure
 * @param name Test case name
 * @param description Test case description
 * @param result Test result
 * @param execution_time_ms Execution time in milliseconds
 * @param error_message Error message (NULL if no error)
 * @retval UavcanError Error code
 */
UavcanError uavcanTestAddResult(UavcanTestSuite* suite, 
                               const char* name,
                               const char* description,
                               UavcanTestResult result,
                               uint32_t execution_time_ms,
                               const char* error_message);

/**
 * @brief Finalize test suite and calculate statistics
 * @param suite Pointer to test suite structure
 * @retval UavcanError Error code
 */
UavcanError uavcanTestFinalize(UavcanTestSuite* suite);

/**
 * @brief Test message sending performance
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @param count Number of messages to send
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestSendMessages(UavcanTestSuite* suite, UavcanNode* node, uint32_t count);

/**
 * @brief Test message latency measurement
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestMeasureLatency(UavcanTestSuite* suite, UavcanNode* node);

/**
 * @brief Test system under stress conditions
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @param duration_sec Duration of stress test in seconds
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestStressTest(UavcanTestSuite* suite, UavcanNode* node, uint32_t duration_sec);

/**
 * @brief Test interoperability with standard UAVCAN messages
 * @param suite Pointer to test suite structure
 * @param node Pointer to initialized node
 * @retval UavcanTestResult Test result
 */
UavcanTestResult uavcanTestInteroperability(UavcanTestSuite* suite, UavcanNode* node);

/**
 * @brief Run comprehensive system-level tests
 * @param suite Pointer to test suite structure
 * @param interface Pointer to network interface for testing
 * @retval UavcanError Error code
 */
UavcanError uavcanTestRunSystemTests(UavcanTestSuite* suite, NetInterface* interface);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_TEST_H */
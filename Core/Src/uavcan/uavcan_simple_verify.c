/**
 * @file uavcan_simple_verify.c
 * @brief Simple UAVCAN Requirements Verification
 * 
 * Lightweight verification tests that fit within CLI buffer constraints
 * and don't cause memory issues.
 */

/* Includes ------------------------------------------------------------------*/
#include "uavcan/uavcan_test.h"
#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_transport.h"
#include "core/net.h"
#include "cmsis_os.h"

#include <string.h>
#include <stdio.h>

/* Private defines -----------------------------------------------------------*/
#define SIMPLE_TEST_NODE_ID     42

/* Private variables ---------------------------------------------------------*/
static UavcanNode simple_test_node;
static char simple_error_msg[64];

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Simple requirements verification test
 * @param interface Pointer to network interface
 * @retval UavcanError Error code
 */
UavcanError uavcanSimpleVerify(NetInterface* interface)
{
    UavcanError error;
    uint32_t passed = 0;
    uint32_t failed = 0;
    
    printf("UAVCAN Simple Verification Starting...\r\n");
    
    // Test 1: Node initialization
    error = uavcanNodeInit(&simple_test_node, interface);
    if (error == UAVCAN_ERROR_NONE) {
        printf("REQ1: Node Init - PASS\r\n");
        passed++;
    } else {
        printf("REQ1: Node Init - FAIL\r\n");
        failed++;
        goto cleanup;
    }
    
    // Test 2: Node ID setting
    error = uavcanNodeSetNodeId(&simple_test_node, SIMPLE_TEST_NODE_ID);
    if (error == UAVCAN_ERROR_NONE && 
        uavcanNodeGetNodeId(&simple_test_node) == SIMPLE_TEST_NODE_ID) {
        printf("REQ2: Node ID - PASS\r\n");
        passed++;
    } else {
        printf("REQ2: Node ID - FAIL\r\n");
        failed++;
    }
    
    // Test 3: Node start/stop
    error = uavcanNodeStart(&simple_test_node);
    if (error == UAVCAN_ERROR_NONE && uavcanNodeIsStarted(&simple_test_node)) {
        printf("REQ3: Node Start - PASS\r\n");
        passed++;
        
        error = uavcanNodeStop(&simple_test_node);
        if (error == UAVCAN_ERROR_NONE && !uavcanNodeIsStarted(&simple_test_node)) {
            printf("REQ4: Node Stop - PASS\r\n");
            passed++;
        } else {
            printf("REQ4: Node Stop - FAIL\r\n");
            failed++;
        }
    } else {
        printf("REQ3: Node Start - FAIL\r\n");
        printf("REQ4: Node Stop - SKIP\r\n");
        failed += 2;
    }
    
    // Test 4: Health and mode setting
    error = uavcanNodeSetHealth(&simple_test_node, UAVCAN_NODE_HEALTH_NOMINAL);
    if (error == UAVCAN_ERROR_NONE) {
        error = uavcanNodeSetMode(&simple_test_node, UAVCAN_NODE_MODE_OPERATIONAL);
        if (error == UAVCAN_ERROR_NONE) {
            printf("REQ5: Health/Mode - PASS\r\n");
            passed++;
        } else {
            printf("REQ5: Health/Mode - FAIL\r\n");
            failed++;
        }
    } else {
        printf("REQ5: Health/Mode - FAIL\r\n");
        failed++;
    }
    
    // Test 5: Transport integration
    if (uavcanTransportIsInitialized(&simple_test_node.transport)) {
        printf("REQ6: Transport - PASS\r\n");
        passed++;
    } else {
        printf("REQ6: Transport - FAIL\r\n");
        failed++;
    }
    
    // Test 6: Memory management
    void* test_ptr = uavcanNodeMemoryAllocate(&simple_test_node, 64);
    if (test_ptr != NULL) {
        printf("REQ7: Memory - PASS\r\n");
        passed++;
        uavcanNodeMemoryFree(&simple_test_node, 64, test_ptr);
    } else {
        printf("REQ7: Memory - FAIL\r\n");
        failed++;
    }
    
cleanup:
    // Clean up
    if (uavcanNodeIsInitialized(&simple_test_node)) {
        uavcanNodeDeinit(&simple_test_node);
    }
    
    // Print summary
    printf("Verification Complete:\r\n");
    printf("Passed: %lu, Failed: %lu\r\n", passed, failed);
    printf("Status: %s\r\n", (failed == 0) ? "PASS" : "FAIL");
    
    return (failed == 0) ? UAVCAN_ERROR_NONE : UAVCAN_ERROR_PROTOCOL_ERROR;
}
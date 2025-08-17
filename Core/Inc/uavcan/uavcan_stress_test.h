#ifndef UAVCAN_STRESS_TEST_H
#define UAVCAN_STRESS_TEST_H

#include "uavcan_integration.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Run UAVCAN stress test with high message loads
 * 
 * This function performs a comprehensive stress test of the UAVCAN system
 * by generating high loads of messages with various priorities and payload sizes.
 * The test validates:
 * - Message priority handling under load
 * - Queue behavior with overflow conditions
 * - System stability during sustained high traffic
 * - Message latency and throughput
 * - Memory usage and leak detection
 * 
 * @param ctx Pointer to initialized and started UAVCAN integration context
 * @return true if stress test passes, false if system shows instability
 */
bool uavcanRunStressTest(UavcanIntegrationContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_STRESS_TEST_H
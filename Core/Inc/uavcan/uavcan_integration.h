#ifndef UAVCAN_INTEGRATION_H
#define UAVCAN_INTEGRATION_H

#include "uavcan_common.h"
#include "uavcan_types.h"
#include "uavcan_tasks.h"
#include "uavcan_udp_transport.h"
#include "uavcan_priority_queue.h"
#include "uavcan_heartbeat_service.h"
#include "uavcan_system_stability.h"
#include "uavcan_config.h"
#include "uavcan_cli_commands.h"
#include "uavcan_error_handler.h"

// CycloneTCP includes
#include "core/net.h"

#ifdef __cplusplus
extern "C" {
#endif

// UAVCAN Integration Context - Main system context
typedef struct {
    // Core components
    UavcanNodeContext node_context;
    UavcanTaskContext task_context;
    UavcanUdpTransport udp_transport;
    UavcanPriorityQueue priority_queue;
    UavcanHeartbeatService heartbeat_service;
    UavcanStabilityManager stability_manager;
    UavcanConfigContext config_context;
    UavcanErrorHandler error_handler;
    
    // System state
    bool initialized;
    bool started;
    NetInterface* net_interface;
    
    // Statistics
    uint32_t init_time_ms;
    uint32_t start_time_ms;
} UavcanIntegrationContext;

/**
 * @brief Initialize the UAVCAN subsystem
 * @param ctx Pointer to integration context
 * @param net_interface Network interface to use
 * @param node_id Initial node ID (0 for dynamic allocation)
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanIntegrationInit(UavcanIntegrationContext* ctx, 
                                 NetInterface* net_interface,
                                 uint8_t node_id);

/**
 * @brief Start the UAVCAN subsystem
 * @param ctx Pointer to integration context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanIntegrationStart(UavcanIntegrationContext* ctx);

/**
 * @brief Stop the UAVCAN subsystem
 * @param ctx Pointer to integration context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanIntegrationStop(UavcanIntegrationContext* ctx);

/**
 * @brief Deinitialize the UAVCAN subsystem
 * @param ctx Pointer to integration context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanIntegrationDeinit(UavcanIntegrationContext* ctx);

/**
 * @brief Check if UAVCAN subsystem is ready
 * @param ctx Pointer to integration context
 * @return true if ready, false otherwise
 */
bool uavcanIntegrationIsReady(const UavcanIntegrationContext* ctx);

/**
 * @brief Get the global UAVCAN integration context
 * @return Pointer to global context or NULL if not initialized
 */
UavcanIntegrationContext* uavcanIntegrationGetContext(void);

/**
 * @brief Register UAVCAN CLI commands with the command console
 * @param ctx Pointer to integration context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanIntegrationRegisterCommands(UavcanIntegrationContext* ctx);

/**
 * @brief Periodic update function (should be called from main loop or timer)
 * @param ctx Pointer to integration context
 */
void uavcanIntegrationUpdate(UavcanIntegrationContext* ctx);

/**
 * @brief Get integration status as formatted string
 * @param ctx Pointer to integration context
 * @param buffer Buffer to store status string
 * @param buffer_size Size of buffer
 * @return Number of characters written to buffer
 */
size_t uavcanIntegrationGetStatusString(const UavcanIntegrationContext* ctx,
                                       char* buffer,
                                       size_t buffer_size);

/**
 * @brief Test UAVCAN integration with real network interface
 * @param net_interface Network interface to test with
 * @return true if test passes, false otherwise
 */
bool uavcanSystemIntegrationTest(NetInterface* net_interface);

/**
 * @brief Test UAVCAN task priorities and configuration
 * @return true if configuration is valid, false otherwise
 */
bool uavcanTestTaskPriorities(void);

/**
 * @brief Test UAVCAN memory usage and resource allocation
 * @return true if memory usage is acceptable, false otherwise
 */
bool uavcanTestMemoryUsage(void);

/**
 * @brief Run comprehensive UAVCAN test suite
 * @return true if all tests pass, false otherwise
 */
bool uavcanRunComprehensiveTests(void);

/**
 * @brief Run UAVCAN stress test with high message loads
 * @param ctx Pointer to UAVCAN integration context
 * @return true if stress test passes, false otherwise
 */
bool uavcanRunStressTest(UavcanIntegrationContext* ctx);

/**
 * @brief Validate all UAVCAN requirements from specification
 * @param ctx Pointer to UAVCAN integration context
 * @return true if all requirements are met, false otherwise
 */
bool uavcanValidateAllRequirements(UavcanIntegrationContext* ctx);

/**
 * @brief Run complete validation suite (requirements + tests)
 * @param ctx Pointer to UAVCAN integration context
 * @return true if complete validation passes, false otherwise
 */
bool uavcanRunCompleteValidation(UavcanIntegrationContext* ctx);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_INTEGRATION_H
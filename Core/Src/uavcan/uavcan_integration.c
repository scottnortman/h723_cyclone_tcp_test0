#include "uavcan/uavcan_integration.h"
#include "uavcan/uavcan_node.h"
#include "uavcan/uavcan_message_handler.h"
#include "uavcan/uavcan_error_handler.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

// Global integration context
static UavcanIntegrationContext g_uavcan_ctx = {0};
static bool g_context_initialized = false;

UavcanError uavcanIntegrationInit(UavcanIntegrationContext* ctx, 
                                 NetInterface* net_interface,
                                 uint8_t node_id)
{
    if (!ctx || !net_interface) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    // Clear context
    memset(ctx, 0, sizeof(UavcanIntegrationContext));
    
    UAVCAN_INFO_PRINT("Initializing UAVCAN subsystem...");
    
    // Store network interface reference
    ctx->net_interface = net_interface;
    ctx->init_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    UavcanError error;
    
    // Initialize node context
    error = uavcanNodeInit(&ctx->node_context, node_id);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to initialize node context: %s", uavcanErrorToString(error));
        return error;
    }
    
    // Initialize configuration context
    error = uavcanConfigInit(&ctx->config_context);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to initialize config context: %s", uavcanErrorToString(error));
        return error;
    }
    
    // Set default configuration
    UavcanConfig default_config = {
        .node_id = node_id,
        .heartbeat_interval_ms = UAVCAN_HEARTBEAT_INTERVAL_MS,
        .udp_port = UAVCAN_UDP_PORT_DEFAULT,
        .monitor_enabled = false,
        .log_level = UAVCAN_LOG_LEVEL_INFO
    };
    strncpy(default_config.multicast_addr, UAVCAN_MULTICAST_ADDR, sizeof(default_config.multicast_addr) - 1);
    
    error = uavcanConfigSet(&ctx->config_context, &default_config);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to set default configuration: %s", uavcanErrorToString(error));
        return error;
    }
    
    // Initialize UDP transport
    error = uavcanUdpTransportInit(&ctx->udp_transport, 
                                   net_interface,
                                   default_config.udp_port,
                                   default_config.multicast_addr);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to initialize UDP transport: %s", uavcanErrorToString(error));
        return error;
    }
    
    // Initialize priority queue
    error = uavcanPriorityQueueInit(&ctx->priority_queue);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to initialize priority queue: %s", uavcanErrorToString(error));
        uavcanUdpTransportDeinit(&ctx->udp_transport);
        return error;
    }
    
    // Initialize heartbeat service
    error = uavcanHeartbeatInit(&ctx->heartbeat_service, &ctx->node_context);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to initialize heartbeat service: %s", uavcanErrorToString(error));
        uavcanPriorityQueueDeinit(&ctx->priority_queue);
        uavcanUdpTransportDeinit(&ctx->udp_transport);
        return error;
    }
    
    // Set heartbeat interval
    error = uavcanHeartbeatSetInterval(&ctx->heartbeat_service, default_config.heartbeat_interval_ms);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to set heartbeat interval: %s", uavcanErrorToString(error));
    }
    
    // Initialize error handler
    error = uavcanErrorHandlerInit(&ctx->error_handler, UAVCAN_LOG_LEVEL_INFO);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to initialize error handler: %s", uavcanErrorToString(error));
        uavcanHeartbeatReset(&ctx->heartbeat_service);
        uavcanPriorityQueueDeinit(&ctx->priority_queue);
        uavcanUdpTransportDeinit(&ctx->udp_transport);
        return error;
    }
    
    // Initialize system stability manager
    error = uavcanStabilityInit(&ctx->stability_manager, &ctx->error_handler);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to initialize stability manager: %s", uavcanErrorToString(error));
        uavcanHeartbeatReset(&ctx->heartbeat_service);
        uavcanPriorityQueueDeinit(&ctx->priority_queue);
        uavcanUdpTransportDeinit(&ctx->udp_transport);
        return error;
    }
    
    // Initialize task context
    error = uavcanTasksInit(&ctx->task_context, 
                           &ctx->node_context,
                           &ctx->priority_queue,
                           &ctx->udp_transport);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to initialize task context: %s", uavcanErrorToString(error));
        uavcanStabilityDeinit(&ctx->stability_manager);
        uavcanHeartbeatReset(&ctx->heartbeat_service);
        uavcanPriorityQueueDeinit(&ctx->priority_queue);
        uavcanUdpTransportDeinit(&ctx->udp_transport);
        return error;
    }
    
    ctx->initialized = true;
    
    // Set global context reference
    if (!g_context_initialized) {
        memcpy(&g_uavcan_ctx, ctx, sizeof(UavcanIntegrationContext));
        g_context_initialized = true;
    }
    
    UAVCAN_INFO_PRINT("UAVCAN subsystem initialized successfully");
    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanIntegrationStart(UavcanIntegrationContext* ctx)
{
    if (!ctx || !ctx->initialized) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    if (ctx->started) {
        UAVCAN_WARN_PRINT("UAVCAN subsystem already started");
        return UAVCAN_ERROR_NONE;
    }
    
    UAVCAN_INFO_PRINT("Starting UAVCAN subsystem...");
    
    ctx->start_time_ms = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    UavcanError error;
    
    // Start node
    error = uavcanNodeStart(&ctx->node_context);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to start node: %s", uavcanErrorToString(error));
        return error;
    }
    
    // Start tasks
    error = uavcanTasksStart(&ctx->task_context);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to start tasks: %s", uavcanErrorToString(error));
        uavcanNodeStop(&ctx->node_context);
        return error;
    }
    
    // Register tasks with stability manager
    error = uavcanStabilityRegisterTask(&ctx->stability_manager,
                                       ctx->task_context.node_task_handle,
                                       "UAVCAN_Node",
                                       5000);  // 5 second heartbeat
    if (UAVCAN_FAILED(error)) {
        UAVCAN_WARN_PRINT("Failed to register node task with stability manager");
    }
    
    error = uavcanStabilityRegisterTask(&ctx->stability_manager,
                                       ctx->task_context.tx_task_handle,
                                       "UAVCAN_TX",
                                       3000);  // 3 second heartbeat
    if (UAVCAN_FAILED(error)) {
        UAVCAN_WARN_PRINT("Failed to register TX task with stability manager");
    }
    
    error = uavcanStabilityRegisterTask(&ctx->stability_manager,
                                       ctx->task_context.rx_task_handle,
                                       "UAVCAN_RX",
                                       3000);  // 3 second heartbeat
    if (UAVCAN_FAILED(error)) {
        UAVCAN_WARN_PRINT("Failed to register RX task with stability manager");
    }
    
    // Start heartbeat service
    error = uavcanHeartbeatStart(&ctx->heartbeat_service);
    if (UAVCAN_FAILED(error)) {
        UAVCAN_ERROR_PRINT("Failed to start heartbeat service: %s", uavcanErrorToString(error));
        uavcanTasksStop(&ctx->task_context);
        uavcanNodeStop(&ctx->node_context);
        return error;
    }
    
    ctx->started = true;
    
    UAVCAN_INFO_PRINT("UAVCAN subsystem started successfully");
    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanIntegrationStop(UavcanIntegrationContext* ctx)
{
    if (!ctx || !ctx->initialized) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    if (!ctx->started) {
        UAVCAN_WARN_PRINT("UAVCAN subsystem already stopped");
        return UAVCAN_ERROR_NONE;
    }
    
    UAVCAN_INFO_PRINT("Stopping UAVCAN subsystem...");
    
    // Stop heartbeat service
    uavcanHeartbeatStop(&ctx->heartbeat_service);
    
    // Stop tasks
    uavcanTasksStop(&ctx->task_context);
    
    // Stop node
    uavcanNodeStop(&ctx->node_context);
    
    ctx->started = false;
    
    UAVCAN_INFO_PRINT("UAVCAN subsystem stopped");
    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanIntegrationDeinit(UavcanIntegrationContext* ctx)
{
    if (!ctx) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    if (!ctx->initialized) {
        return UAVCAN_ERROR_NONE;
    }
    
    UAVCAN_INFO_PRINT("Deinitializing UAVCAN subsystem...");
    
    // Stop if running
    if (ctx->started) {
        uavcanIntegrationStop(ctx);
    }
    
    // Deinitialize components in reverse order
    uavcanStabilityDeinit(&ctx->stability_manager);
    uavcanErrorHandlerDeinit(&ctx->error_handler);
    uavcanHeartbeatReset(&ctx->heartbeat_service);
    uavcanPriorityQueueDeinit(&ctx->priority_queue);
    uavcanUdpTransportDeinit(&ctx->udp_transport);
    uavcanConfigDeinit(&ctx->config_context);
    uavcanNodeReset(&ctx->node_context);
    
    // Clear context
    memset(ctx, 0, sizeof(UavcanIntegrationContext));
    
    // Clear global context if it matches
    if (g_context_initialized && ctx == &g_uavcan_ctx) {
        memset(&g_uavcan_ctx, 0, sizeof(UavcanIntegrationContext));
        g_context_initialized = false;
    }
    
    UAVCAN_INFO_PRINT("UAVCAN subsystem deinitialized");
    return UAVCAN_ERROR_NONE;
}

bool uavcanIntegrationIsReady(const UavcanIntegrationContext* ctx)
{
    if (!ctx || !ctx->initialized || !ctx->started) {
        return false;
    }
    
    // Check if UDP transport is ready
    if (!uavcanUdpTransportIsReady(&ctx->udp_transport)) {
        return false;
    }
    
    // Check if tasks are running
    if (!uavcanTasksAreRunning(&ctx->task_context)) {
        return false;
    }
    
    // Check system stability
    if (!uavcanStabilityIsOperational(&ctx->stability_manager)) {
        return false;
    }
    
    return true;
}

UavcanIntegrationContext* uavcanIntegrationGetContext(void)
{
    if (g_context_initialized) {
        return &g_uavcan_ctx;
    }
    return NULL;
}

UavcanError uavcanIntegrationRegisterCommands(UavcanIntegrationContext* ctx)
{
    if (!ctx || !ctx->initialized) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }
    
    // Set CLI command contexts
    uavcanCliSetNodeContext(&ctx->node_context);
    uavcanCliSetHeartbeatService(&ctx->heartbeat_service);
    uavcanCliSetConfigContext(&ctx->config_context);
    
    // Register all UAVCAN CLI commands
    vRegisterUavcanCLICommands();
    
    UAVCAN_INFO_PRINT("UAVCAN CLI commands registered");
    return UAVCAN_ERROR_NONE;
}

void uavcanIntegrationUpdate(UavcanIntegrationContext* ctx)
{
    if (!ctx || !ctx->initialized) {
        return;
    }
    
    // Update stability manager
    uavcanStabilityUpdate(&ctx->stability_manager);
    
    // Update node uptime
    uint32_t current_time = xTaskGetTickCount() * portTICK_PERIOD_MS;
    ctx->node_context.uptime_sec = (current_time - ctx->start_time_ms) / 1000;
}

size_t uavcanIntegrationGetStatusString(const UavcanIntegrationContext* ctx,
                                       char* buffer,
                                       size_t buffer_size)
{
    if (!ctx || !buffer || buffer_size == 0) {
        return 0;
    }
    
    size_t written = 0;
    
    written += snprintf(buffer + written, buffer_size - written,
                       "UAVCAN Integration Status:\n");
    
    written += snprintf(buffer + written, buffer_size - written,
                       "  Initialized: %s\n", ctx->initialized ? "Yes" : "No");
    
    written += snprintf(buffer + written, buffer_size - written,
                       "  Started: %s\n", ctx->started ? "Yes" : "No");
    
    written += snprintf(buffer + written, buffer_size - written,
                       "  Ready: %s\n", uavcanIntegrationIsReady(ctx) ? "Yes" : "No");
    
    if (ctx->initialized) {
        written += snprintf(buffer + written, buffer_size - written,
                           "  Node ID: %u\n", ctx->node_context.node_id);
        
        written += snprintf(buffer + written, buffer_size - written,
                           "  Node Health: %s\n", 
                           uavcanNodeHealthToString(ctx->node_context.health));
        
        written += snprintf(buffer + written, buffer_size - written,
                           "  Node Mode: %s\n", 
                           uavcanNodeModeToString(ctx->node_context.mode));
        
        written += snprintf(buffer + written, buffer_size - written,
                           "  Uptime: %lu seconds\n", ctx->node_context.uptime_sec);
        
        // Add task status
        char task_status[256];
        uavcanTasksGetStatusString(&ctx->task_context, task_status, sizeof(task_status));
        written += snprintf(buffer + written, buffer_size - written,
                           "  %s", task_status);
    }
    
    return written;
}
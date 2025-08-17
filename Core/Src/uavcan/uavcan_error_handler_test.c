#include "uavcan/uavcan_error_handler.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

// Mock FreeRTOS functions for testing
static uint32_t mock_tick_count = 0;
uint32_t xTaskGetTickCount(void) { return mock_tick_count; }
void vTaskDelay(uint32_t ticks) { mock_tick_count += ticks; }

// Test callback function
static UavcanErrorContext last_error_context;
static bool callback_called = false;

static void test_error_callback(const UavcanErrorContext* error_ctx)
{
    if (error_ctx != NULL) {
        last_error_context = *error_ctx;
        callback_called = true;
    }
}

/**
 * Test error handler initialization
 */
static void test_error_handler_init(void)
{
    printf("Testing error handler initialization...\n");
    
    UavcanErrorHandler handler;
    
    // Test successful initialization
    UavcanError result = uavcanErrorHandlerInit(&handler, UAVCAN_LOG_LEVEL_INFO);
    assert(result == UAVCAN_ERROR_NONE);
    assert(handler.min_log_level == UAVCAN_LOG_LEVEL_INFO);
    assert(handler.auto_recovery_enabled == true);
    assert(handler.max_recovery_attempts == 3);
    assert(handler.statistics.total_errors == 0);
    
    // Test NULL parameter
    result = uavcanErrorHandlerInit(NULL, UAVCAN_LOG_LEVEL_INFO);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    printf("✓ Error handler initialization tests passed\n");
}

/**
 * Test error logging functionality
 */
static void test_error_logging(void)
{
    printf("Testing error logging...\n");
    
    UavcanErrorHandler handler;
    uavcanErrorHandlerInit(&handler, UAVCAN_LOG_LEVEL_WARNING);
    
    // Set up callback
    handler.error_callback = test_error_callback;
    callback_called = false;
    
    // Test logging an error above minimum level
    mock_tick_count = 1000;
    uavcanLogError(&handler, UAVCAN_ERROR_SEND_FAILED, UAVCAN_LOG_LEVEL_ERROR,
                   "test_function", 123, "Test error message", 0xDEADBEEF);
    
    // Verify statistics were updated
    assert(handler.statistics.total_errors == 1);
    assert(handler.statistics.errors_by_type[UAVCAN_ERROR_SEND_FAILED] == 1);
    assert(handler.statistics.last_error_code == UAVCAN_ERROR_SEND_FAILED);
    assert(callback_called == true);
    
    // Verify callback received correct data
    assert(last_error_context.error_code == UAVCAN_ERROR_SEND_FAILED);
    assert(last_error_context.severity == UAVCAN_LOG_LEVEL_ERROR);
    assert(last_error_context.line_number == 123);
    assert(last_error_context.additional_data == 0xDEADBEEF);
    
    // Test logging below minimum level (should be ignored)
    callback_called = false;
    uint32_t prev_total = handler.statistics.total_errors;
    uavcanLogError(&handler, UAVCAN_ERROR_TIMEOUT, UAVCAN_LOG_LEVEL_DEBUG,
                   "test_function", 456, "Debug message", 0);
    
    assert(handler.statistics.total_errors == prev_total);
    assert(callback_called == false);
    
    // Test critical error counting
    uavcanLogError(&handler, UAVCAN_ERROR_INIT_FAILED, UAVCAN_LOG_LEVEL_CRITICAL,
                   "test_function", 789, "Critical error", 0);
    
    assert(handler.statistics.critical_errors == 1);
    
    printf("✓ Error logging tests passed\n");
}

/**
 * Test error recovery functionality
 */
static void test_error_recovery(void)
{
    printf("Testing error recovery...\n");
    
    UavcanErrorHandler handler;
    uavcanErrorHandlerInit(&handler, UAVCAN_LOG_LEVEL_DEBUG);
    
    // Test recoverable errors
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_NETWORK_UNAVAILABLE) == true);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_QUEUE_FULL) == true);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_TIMEOUT) == true);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_SEND_FAILED) == true);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_RECEIVE_FAILED) == true);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_TRANSPORT_ERROR) == true);
    
    // Test non-recoverable errors
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_INIT_FAILED) == false);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_INVALID_CONFIG) == false);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_INVALID_PARAMETER) == false);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_MEMORY_ALLOCATION) == false);
    assert(uavcanIsRecoverableError(UAVCAN_ERROR_NODE_ID_CONFLICT) == false);
    
    // Test recovery attempts
    UavcanError result = uavcanRecoverFromError(&handler, UAVCAN_ERROR_NETWORK_UNAVAILABLE);
    assert(result == UAVCAN_ERROR_NONE);
    
    result = uavcanRecoverFromError(&handler, UAVCAN_ERROR_QUEUE_FULL);
    assert(result == UAVCAN_ERROR_NONE);
    
    result = uavcanRecoverFromError(&handler, UAVCAN_ERROR_INIT_FAILED);
    assert(result == UAVCAN_ERROR_INIT_FAILED); // Non-recoverable
    
    // Test NULL parameter
    result = uavcanRecoverFromError(NULL, UAVCAN_ERROR_TIMEOUT);
    assert(result == UAVCAN_ERROR_INVALID_PARAMETER);
    
    printf("✓ Error recovery tests passed\n");
}

/**
 * Test automatic recovery during logging
 */
static void test_automatic_recovery(void)
{
    printf("Testing automatic recovery...\n");
    
    UavcanErrorHandler handler;
    uavcanErrorHandlerInit(&handler, UAVCAN_LOG_LEVEL_DEBUG);
    
    // Enable automatic recovery
    handler.auto_recovery_enabled = true;
    handler.max_recovery_attempts = 2;
    
    // Log a recoverable error
    uavcanLogError(&handler, UAVCAN_ERROR_QUEUE_FULL, UAVCAN_LOG_LEVEL_WARNING,
                   "test_function", 100, "Queue full error", 0);
    
    // Verify recovery was attempted
    assert(handler.statistics.recovery_attempts == 1);
    assert(handler.statistics.successful_recoveries == 1);
    
    // Test recovery attempt limit
    handler.statistics.recovery_attempts = handler.max_recovery_attempts;
    uint32_t prev_attempts = handler.statistics.recovery_attempts;
    
    uavcanLogError(&handler, UAVCAN_ERROR_TIMEOUT, UAVCAN_LOG_LEVEL_WARNING,
                   "test_function", 200, "Timeout error", 0);
    
    // Should not attempt recovery due to limit
    assert(handler.statistics.recovery_attempts == prev_attempts);
    
    printf("✓ Automatic recovery tests passed\n");
}

/**
 * Test error string functionality
 */
static void test_error_strings(void)
{
    printf("Testing error strings...\n");
    
    // Test known error codes
    assert(strcmp(uavcanGetErrorString(UAVCAN_ERROR_NONE), "No error") == 0);
    assert(strcmp(uavcanGetErrorString(UAVCAN_ERROR_INIT_FAILED), "Initialization failed") == 0);
    assert(strcmp(uavcanGetErrorString(UAVCAN_ERROR_NETWORK_UNAVAILABLE), "Network unavailable") == 0);
    assert(strcmp(uavcanGetErrorString(UAVCAN_ERROR_SEND_FAILED), "Send operation failed") == 0);
    
    // Test unknown error code
    assert(strcmp(uavcanGetErrorString((UavcanError)999), "Unknown error") == 0);
    
    printf("✓ Error string tests passed\n");
}

/**
 * Test error statistics functionality
 */
static void test_error_statistics(void)
{
    printf("Testing error statistics...\n");
    
    UavcanErrorHandler handler;
    uavcanErrorHandlerInit(&handler, UAVCAN_LOG_LEVEL_DEBUG);
    
    // Test initial statistics
    const UavcanErrorStatistics* stats = uavcanGetErrorStatistics(&handler);
    assert(stats != NULL);
    assert(stats->total_errors == 0);
    assert(stats->critical_errors == 0);
    
    // Log some errors
    uavcanLogError(&handler, UAVCAN_ERROR_SEND_FAILED, UAVCAN_LOG_LEVEL_ERROR,
                   "test", 1, "Error 1", 0);
    uavcanLogError(&handler, UAVCAN_ERROR_SEND_FAILED, UAVCAN_LOG_LEVEL_ERROR,
                   "test", 2, "Error 2", 0);
    uavcanLogError(&handler, UAVCAN_ERROR_TIMEOUT, UAVCAN_LOG_LEVEL_CRITICAL,
                   "test", 3, "Critical error", 0);
    
    // Verify statistics
    stats = uavcanGetErrorStatistics(&handler);
    assert(stats->total_errors == 3);
    assert(stats->errors_by_type[UAVCAN_ERROR_SEND_FAILED] == 2);
    assert(stats->errors_by_type[UAVCAN_ERROR_TIMEOUT] == 1);
    assert(stats->critical_errors == 1);
    assert(stats->last_error_code == UAVCAN_ERROR_TIMEOUT);
    
    // Test statistics reset
    uavcanResetErrorStatistics(&handler);
    stats = uavcanGetErrorStatistics(&handler);
    assert(stats->total_errors == 0);
    assert(stats->critical_errors == 0);
    
    // Test NULL parameter
    assert(uavcanGetErrorStatistics(NULL) == NULL);
    
    printf("✓ Error statistics tests passed\n");
}

/**
 * Test convenience macros
 */
static void test_convenience_macros(void)
{
    printf("Testing convenience macros...\n");
    
    UavcanErrorHandler handler;
    uavcanErrorHandlerInit(&handler, UAVCAN_LOG_LEVEL_DEBUG);
    
    // Test all macro levels
    UAVCAN_LOG_DEBUG(&handler, UAVCAN_ERROR_NONE, "Debug message", 1);
    UAVCAN_LOG_INFO(&handler, UAVCAN_ERROR_NONE, "Info message", 2);
    UAVCAN_LOG_WARNING(&handler, UAVCAN_ERROR_TIMEOUT, "Warning message", 3);
    UAVCAN_LOG_ERROR(&handler, UAVCAN_ERROR_SEND_FAILED, "Error message", 4);
    UAVCAN_LOG_CRITICAL(&handler, UAVCAN_ERROR_INIT_FAILED, "Critical message", 5);
    
    // Verify statistics
    const UavcanErrorStatistics* stats = uavcanGetErrorStatistics(&handler);
    assert(stats->total_errors == 5);
    assert(stats->critical_errors == 1);
    
    printf("✓ Convenience macro tests passed\n");
}

/**
 * Run all error handler tests
 */
void uavcanErrorHandlerRunTests(void)
{
    printf("=== UAVCAN Error Handler Tests ===\n");
    
    test_error_handler_init();
    test_error_logging();
    test_error_recovery();
    test_automatic_recovery();
    test_error_strings();
    test_error_statistics();
    test_convenience_macros();
    
    printf("=== All Error Handler Tests Passed ===\n");
}

// Main function for standalone testing
#ifdef UAVCAN_ERROR_HANDLER_TEST_STANDALONE
int main(void)
{
    uavcanErrorHandlerRunTests();
    return 0;
}
#endif
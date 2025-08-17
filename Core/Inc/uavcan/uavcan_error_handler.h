#ifndef UAVCAN_ERROR_HANDLER_H
#define UAVCAN_ERROR_HANDLER_H

#include "uavcan_types.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error severity levels for logging
typedef enum {
    UAVCAN_LOG_LEVEL_DEBUG = 0,
    UAVCAN_LOG_LEVEL_INFO = 1,
    UAVCAN_LOG_LEVEL_WARNING = 2,
    UAVCAN_LOG_LEVEL_ERROR = 3,
    UAVCAN_LOG_LEVEL_CRITICAL = 4
} UavcanLogLevel;

// Error context structure for detailed error information
typedef struct {
    UavcanError error_code;
    UavcanLogLevel severity;
    uint32_t timestamp_ms;
    const char* function_name;
    uint32_t line_number;
    const char* description;
    uint32_t additional_data;
} UavcanErrorContext;

// Error statistics structure
typedef struct {
    uint32_t total_errors;
    uint32_t errors_by_type[UAVCAN_ERROR_TRANSPORT_ERROR + 1];
    uint32_t critical_errors;
    uint32_t recovery_attempts;
    uint32_t successful_recoveries;
    uint64_t last_error_timestamp;
    UavcanError last_error_code;
} UavcanErrorStatistics;

// Error handler callback function type
typedef void (*UavcanErrorCallback)(const UavcanErrorContext* error_ctx);

// Error handler configuration
typedef struct {
    UavcanLogLevel min_log_level;
    bool auto_recovery_enabled;
    uint32_t max_recovery_attempts;
    UavcanErrorCallback error_callback;
    UavcanErrorStatistics statistics;
} UavcanErrorHandler;

// Error handling functions
UavcanError uavcanErrorHandlerInit(UavcanErrorHandler* handler, UavcanLogLevel min_level);
void uavcanErrorHandlerDeinit(UavcanErrorHandler* handler);

// Error logging and reporting
void uavcanLogError(UavcanErrorHandler* handler, UavcanError error_code, 
                   UavcanLogLevel severity, const char* function, uint32_t line,
                   const char* description, uint32_t additional_data);

// Error recovery functions
UavcanError uavcanRecoverFromError(UavcanErrorHandler* handler, UavcanError error_code);
bool uavcanIsRecoverableError(UavcanError error_code);
const char* uavcanGetErrorString(UavcanError error_code);

// Error statistics
const UavcanErrorStatistics* uavcanGetErrorStatistics(const UavcanErrorHandler* handler);
void uavcanResetErrorStatistics(UavcanErrorHandler* handler);

// Convenience macros for error logging
#define UAVCAN_LOG_DEBUG(handler, error, desc, data) \
    uavcanLogError(handler, error, UAVCAN_LOG_LEVEL_DEBUG, __FUNCTION__, __LINE__, desc, data)

#define UAVCAN_LOG_INFO(handler, error, desc, data) \
    uavcanLogError(handler, error, UAVCAN_LOG_LEVEL_INFO, __FUNCTION__, __LINE__, desc, data)

#define UAVCAN_LOG_WARNING(handler, error, desc, data) \
    uavcanLogError(handler, error, UAVCAN_LOG_LEVEL_WARNING, __FUNCTION__, __LINE__, desc, data)

#define UAVCAN_LOG_ERROR(handler, error, desc, data) \
    uavcanLogError(handler, error, UAVCAN_LOG_LEVEL_ERROR, __FUNCTION__, __LINE__, desc, data)

#define UAVCAN_LOG_CRITICAL(handler, error, desc, data) \
    uavcanLogError(handler, error, UAVCAN_LOG_LEVEL_CRITICAL, __FUNCTION__, __LINE__, desc, data)

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_ERROR_HANDLER_H
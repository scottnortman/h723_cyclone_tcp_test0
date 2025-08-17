#ifndef UAVCAN_MESSAGE_HANDLER_H
#define UAVCAN_MESSAGE_HANDLER_H

#include "uavcan_types.h"

// For testing without full CycloneTCP stack
#ifndef ERROR_H
typedef enum {
    NO_ERROR = 0,
    ERROR_INVALID_PARAMETER,
    ERROR_OUT_OF_MEMORY,
    ERROR_FAILURE
} error_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create a new UAVCAN message
 * 
 * @param msg Pointer to message structure to initialize
 * @param subject_id Subject ID for the message
 * @param priority Priority level (0-7, where 0 is highest)
 * @param payload Pointer to payload data
 * @param payload_size Size of payload in bytes
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageCreate(UavcanMessage* msg, uint32_t subject_id, 
                           uint8_t priority, const void* payload, size_t payload_size);

/**
 * @brief Destroy a UAVCAN message and free allocated memory
 * 
 * @param msg Pointer to message to destroy
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageDestroy(UavcanMessage* msg);

/**
 * @brief Validate message priority
 * 
 * @param priority Priority value to validate
 * @return bool True if priority is valid (0-7), false otherwise
 */
bool uavcanMessageValidatePriority(uint8_t priority);

/**
 * @brief Validate message subject ID
 * 
 * @param subject_id Subject ID to validate
 * @return bool True if subject ID is valid, false otherwise
 */
bool uavcanMessageValidateSubjectId(uint32_t subject_id);

/**
 * @brief Validate message payload size
 * 
 * @param payload_size Payload size to validate
 * @return bool True if payload size is valid, false otherwise
 */
bool uavcanMessageValidatePayloadSize(size_t payload_size);

/**
 * @brief Validate complete message structure
 * 
 * @param msg Pointer to message to validate
 * @return bool True if message is valid, false otherwise
 */
bool uavcanMessageValidate(const UavcanMessage* msg);

/**
 * @brief Set message timestamp to current time
 * 
 * @param msg Pointer to message
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageSetTimestamp(UavcanMessage* msg);

/**
 * @brief Copy message payload data
 * 
 * @param msg Pointer to message
 * @param payload Pointer to payload data to copy
 * @param payload_size Size of payload data
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageCopyPayload(UavcanMessage* msg, const void* payload, size_t payload_size);

/**
 * @brief Initialize message structure with default values
 * 
 * @param msg Pointer to message to initialize
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageInit(UavcanMessage* msg);

/**
 * @brief Serialize UAVCAN message to UDP payload format
 * 
 * @param msg Pointer to message to serialize
 * @param buffer Pointer to output buffer
 * @param buffer_size Size of output buffer
 * @param out_payload_size Pointer to store actual serialized size
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageSerialize(const UavcanMessage* msg, uint8_t* buffer, 
                              size_t buffer_size, size_t* out_payload_size);

/**
 * @brief Deserialize UDP payload to UAVCAN message format
 * 
 * @param buffer Pointer to input buffer containing serialized data
 * @param buffer_size Size of input buffer
 * @param msg Pointer to message structure to populate
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageDeserialize(const uint8_t* buffer, size_t buffer_size, 
                                UavcanMessage* msg);

/**
 * @brief Create a heartbeat message
 * 
 * @param msg Pointer to message structure to populate
 * @param node_health Current node health status
 * @param node_mode Current node mode
 * @param uptime_sec Node uptime in seconds
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageCreateHeartbeat(UavcanMessage* msg, UavcanNodeHealth node_health,
                                    UavcanNodeMode node_mode, uint32_t uptime_sec);

/**
 * @brief Create a node info message
 * 
 * @param msg Pointer to message structure to populate
 * @param node_name Node name string
 * @param software_version Software version information
 * @param hardware_version Hardware version information
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageCreateNodeInfo(UavcanMessage* msg, const char* node_name,
                                   uint32_t software_version, uint32_t hardware_version);

/**
 * @brief Validate serialized message format
 * 
 * @param buffer Pointer to serialized data
 * @param buffer_size Size of serialized data
 * @return bool True if format is valid, false otherwise
 */
bool uavcanMessageValidateSerialized(const uint8_t* buffer, size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_MESSAGE_HANDLER_H
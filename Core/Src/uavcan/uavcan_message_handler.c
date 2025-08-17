#include "uavcan/uavcan_message_handler.h"
#include <string.h>
#include <stdlib.h>

// Platform-specific includes for timing
#ifdef STM32H7xx_HAL_H
#include "stm32h7xx_hal.h"
#else
#include <time.h>
#endif

/**
 * @brief Get current timestamp in microseconds
 * 
 * @return uint64_t Current timestamp in microseconds
 */
static uint64_t getCurrentTimestampUsec(void)
{
#ifdef STM32H7xx_HAL_H
    // Use HAL tick for STM32 (convert ms to us)
    return (uint64_t)HAL_GetTick() * 1000ULL;
#else
    // Fallback for testing/simulation
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
#endif
}

error_t uavcanMessageInit(UavcanMessage* msg)
{
    if (msg == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize all fields to default values
    memset(msg, 0, sizeof(UavcanMessage));
    
    msg->subject_id = 0;
    msg->priority = CYPHAL_PRIORITY_NOMINAL;  // Default priority
    msg->payload_size = 0;
    msg->payload = NULL;
    msg->timestamp_usec = 0;
    msg->source_node_id = UAVCAN_NODE_ID_UNSET;
    msg->destination_node_id = UAVCAN_NODE_ID_UNSET;
    msg->is_service_request = false;
    msg->is_anonymous = false;

    return NO_ERROR;
}

error_t uavcanMessageCreate(UavcanMessage* msg, uint32_t subject_id, 
                           uint8_t priority, const void* payload, size_t payload_size)
{
    if (msg == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate parameters
    if (!uavcanMessageValidatePriority(priority)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!uavcanMessageValidateSubjectId(subject_id)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!uavcanMessageValidatePayloadSize(payload_size)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (payload_size > 0 && payload == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Initialize message structure
    error_t result = uavcanMessageInit(msg);
    if (result != NO_ERROR) {
        return result;
    }

    // Set message fields
    msg->subject_id = subject_id;
    msg->priority = priority;
    msg->payload_size = payload_size;

    // Copy payload if provided
    if (payload_size > 0 && payload != NULL) {
        result = uavcanMessageCopyPayload(msg, payload, payload_size);
        if (result != NO_ERROR) {
            return result;
        }
    }

    // Set timestamp
    result = uavcanMessageSetTimestamp(msg);
    if (result != NO_ERROR) {
        uavcanMessageDestroy(msg);  // Clean up on error
        return result;
    }

    return NO_ERROR;
}

error_t uavcanMessageDestroy(UavcanMessage* msg)
{
    if (msg == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Free allocated payload memory
    if (msg->payload != NULL) {
        free(msg->payload);
        msg->payload = NULL;
    }

    // Reset all fields
    msg->payload_size = 0;
    msg->subject_id = 0;
    msg->priority = 0;
    msg->timestamp_usec = 0;
    msg->source_node_id = UAVCAN_NODE_ID_UNSET;
    msg->destination_node_id = UAVCAN_NODE_ID_UNSET;
    msg->is_service_request = false;
    msg->is_anonymous = false;

    return NO_ERROR;
}

bool uavcanMessageValidatePriority(uint8_t priority)
{
    return (priority < CYPHAL_PRIORITY_LEVELS);
}

bool uavcanMessageValidateSubjectId(uint32_t subject_id)
{
    return (subject_id <= UAVCAN_SUBJECT_ID_MAX);
}

bool uavcanMessageValidatePayloadSize(size_t payload_size)
{
    return (payload_size <= UAVCAN_MAX_PAYLOAD_SIZE);
}

bool uavcanMessageValidate(const UavcanMessage* msg)
{
    if (msg == NULL) {
        return false;
    }

    // Validate priority
    if (!uavcanMessageValidatePriority(msg->priority)) {
        return false;
    }

    // Validate subject ID
    if (!uavcanMessageValidateSubjectId(msg->subject_id)) {
        return false;
    }

    // Validate payload size
    if (!uavcanMessageValidatePayloadSize(msg->payload_size)) {
        return false;
    }

    // Validate payload consistency
    if (msg->payload_size > 0 && msg->payload == NULL) {
        return false;
    }

    if (msg->payload_size == 0 && msg->payload != NULL) {
        return false;
    }

    // Validate node IDs if set
    if (msg->source_node_id != UAVCAN_NODE_ID_UNSET) {
        if (msg->source_node_id < UAVCAN_NODE_ID_MIN || 
            msg->source_node_id > UAVCAN_NODE_ID_MAX) {
            return false;
        }
    }

    if (msg->destination_node_id != UAVCAN_NODE_ID_UNSET) {
        if (msg->destination_node_id < UAVCAN_NODE_ID_MIN || 
            msg->destination_node_id > UAVCAN_NODE_ID_MAX) {
            return false;
        }
    }

    return true;
}

error_t uavcanMessageSetTimestamp(UavcanMessage* msg)
{
    if (msg == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    msg->timestamp_usec = getCurrentTimestampUsec();
    return NO_ERROR;
}

error_t uavcanMessageCopyPayload(UavcanMessage* msg, const void* payload, size_t payload_size)
{
    if (msg == NULL || (payload_size > 0 && payload == NULL)) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!uavcanMessageValidatePayloadSize(payload_size)) {
        return ERROR_INVALID_PARAMETER;
    }

    // Free existing payload if any
    if (msg->payload != NULL) {
        free(msg->payload);
        msg->payload = NULL;
    }

    msg->payload_size = 0;

    // Allocate and copy new payload if size > 0
    if (payload_size > 0) {
        msg->payload = malloc(payload_size);
        if (msg->payload == NULL) {
            return ERROR_OUT_OF_MEMORY;
        }

        memcpy(msg->payload, payload, payload_size);
        msg->payload_size = payload_size;
    }

    return NO_ERROR;
}

// UAVCAN Standard Subject IDs (as per UAVCAN specification)
#define UAVCAN_SUBJECT_ID_HEARTBEAT     7509
#define UAVCAN_SUBJECT_ID_NODE_INFO     430

// UAVCAN Message serialization helpers
static void serializeU8(uint8_t** ptr, uint8_t value)
{
    **ptr = value;
    (*ptr)++;
}

static void serializeU16(uint8_t** ptr, uint16_t value)
{
    **ptr = (uint8_t)(value & 0xFF);
    (*ptr)++;
    **ptr = (uint8_t)((value >> 8) & 0xFF);
    (*ptr)++;
}

static void serializeU32(uint8_t** ptr, uint32_t value)
{
    **ptr = (uint8_t)(value & 0xFF);
    (*ptr)++;
    **ptr = (uint8_t)((value >> 8) & 0xFF);
    (*ptr)++;
    **ptr = (uint8_t)((value >> 16) & 0xFF);
    (*ptr)++;
    **ptr = (uint8_t)((value >> 24) & 0xFF);
    (*ptr)++;
}

static uint8_t deserializeU8(const uint8_t** ptr)
{
    uint8_t value = **ptr;
    (*ptr)++;
    return value;
}

static uint16_t deserializeU16(const uint8_t** ptr)
{
    uint16_t value = **ptr;
    (*ptr)++;
    value |= ((uint16_t)**ptr) << 8;
    (*ptr)++;
    return value;
}

static uint32_t deserializeU32(const uint8_t** ptr)
{
    uint32_t value = **ptr;
    (*ptr)++;
    value |= ((uint32_t)**ptr) << 8;
    (*ptr)++;
    value |= ((uint32_t)**ptr) << 16;
    (*ptr)++;
    value |= ((uint32_t)**ptr) << 24;
    (*ptr)++;
    return value;
}

error_t uavcanMessageSerialize(const UavcanMessage* msg, uint8_t* buffer, 
                              size_t buffer_size, size_t* out_payload_size)
{
    if (msg == NULL || buffer == NULL || out_payload_size == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    if (!uavcanMessageValidate(msg)) {
        return ERROR_INVALID_PARAMETER;
    }

    // Calculate required buffer size: payload + minimal header overhead
    size_t required_size = msg->payload_size + 16; // Conservative estimate for headers
    if (buffer_size < required_size) {
        return ERROR_INVALID_PARAMETER;
    }

    uint8_t* ptr = buffer;
    
    // Simple serialization format for basic UAVCAN messages
    // This is a simplified format - in a full implementation, this would use
    // proper DSDL serialization with libudpard
    
    // Serialize message metadata
    serializeU32(&ptr, msg->subject_id);
    serializeU8(&ptr, msg->priority);
    serializeU8(&ptr, msg->source_node_id);
    serializeU8(&ptr, msg->destination_node_id);
    serializeU8(&ptr, msg->is_service_request ? 1 : 0);
    serializeU8(&ptr, msg->is_anonymous ? 1 : 0);
    
    // Serialize payload size
    serializeU32(&ptr, (uint32_t)msg->payload_size);
    
    // Serialize payload data
    if (msg->payload_size > 0 && msg->payload != NULL) {
        memcpy(ptr, msg->payload, msg->payload_size);
        ptr += msg->payload_size;
    }
    
    *out_payload_size = (size_t)(ptr - buffer);
    return NO_ERROR;
}

error_t uavcanMessageDeserialize(const uint8_t* buffer, size_t buffer_size, 
                                UavcanMessage* msg)
{
    if (buffer == NULL || msg == NULL || buffer_size < 13) { // Minimum header size
        return ERROR_INVALID_PARAMETER;
    }

    const uint8_t* ptr = buffer;
    const uint8_t* end = buffer + buffer_size;
    
    // Initialize message
    error_t result = uavcanMessageInit(msg);
    if (result != NO_ERROR) {
        return result;
    }
    
    // Deserialize message metadata
    if (ptr + 13 > end) {
        return ERROR_INVALID_PARAMETER;
    }
    
    msg->subject_id = deserializeU32(&ptr);
    msg->priority = deserializeU8(&ptr);
    msg->source_node_id = deserializeU8(&ptr);
    msg->destination_node_id = deserializeU8(&ptr);
    msg->is_service_request = (deserializeU8(&ptr) != 0);
    msg->is_anonymous = (deserializeU8(&ptr) != 0);
    
    // Deserialize payload size
    uint32_t payload_size = deserializeU32(&ptr);
    
    // Validate payload size
    if (payload_size > UAVCAN_MAX_PAYLOAD_SIZE) {
        return ERROR_INVALID_PARAMETER;
    }
    
    if (ptr + payload_size > end) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Copy payload if present
    if (payload_size > 0) {
        result = uavcanMessageCopyPayload(msg, ptr, payload_size);
        if (result != NO_ERROR) {
            return result;
        }
    }
    
    // Set timestamp to current time
    result = uavcanMessageSetTimestamp(msg);
    if (result != NO_ERROR) {
        uavcanMessageDestroy(msg);
        return result;
    }
    
    return NO_ERROR;
}

error_t uavcanMessageCreateHeartbeat(UavcanMessage* msg, UavcanNodeHealth node_health,
                                    UavcanNodeMode node_mode, uint32_t uptime_sec)
{
    if (msg == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Validate health and mode values
    if (node_health > UAVCAN_NODE_HEALTH_WARNING || node_mode > UAVCAN_NODE_MODE_OFFLINE) {
        return ERROR_INVALID_PARAMETER;
    }

    // Create heartbeat payload (simplified format)
    uint8_t heartbeat_payload[8];
    uint8_t* ptr = heartbeat_payload;
    
    serializeU32(&ptr, uptime_sec);
    serializeU8(&ptr, (uint8_t)node_health);
    serializeU8(&ptr, (uint8_t)node_mode);
    serializeU16(&ptr, 0); // Reserved fields
    
    // Create message
    error_t result = uavcanMessageCreate(msg, UAVCAN_SUBJECT_ID_HEARTBEAT, 
                                        CYPHAL_PRIORITY_NOMINAL, 
                                        heartbeat_payload, sizeof(heartbeat_payload));
    
    return result;
}

error_t uavcanMessageCreateNodeInfo(UavcanMessage* msg, const char* node_name,
                                   uint32_t software_version, uint32_t hardware_version)
{
    if (msg == NULL || node_name == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    size_t name_len = strlen(node_name);
    if (name_len > 50) { // Reasonable limit for node name
        return ERROR_INVALID_PARAMETER;
    }

    // Create node info payload (simplified format)
    uint8_t node_info_payload[64];
    uint8_t* ptr = node_info_payload;
    
    serializeU32(&ptr, software_version);
    serializeU32(&ptr, hardware_version);
    serializeU8(&ptr, (uint8_t)name_len);
    
    // Copy node name
    memcpy(ptr, node_name, name_len);
    ptr += name_len;
    
    size_t total_size = (size_t)(ptr - node_info_payload);
    
    // Create message
    error_t result = uavcanMessageCreate(msg, UAVCAN_SUBJECT_ID_NODE_INFO, 
                                        CYPHAL_PRIORITY_LOW, 
                                        node_info_payload, total_size);
    
    return result;
}

bool uavcanMessageValidateSerialized(const uint8_t* buffer, size_t buffer_size)
{
    if (buffer == NULL || buffer_size < 13) { // Minimum header size
        return false;
    }

    const uint8_t* ptr = buffer;
    
    // Check subject ID
    uint32_t subject_id = deserializeU32(&ptr);
    if (subject_id > UAVCAN_SUBJECT_ID_MAX) {
        return false;
    }
    
    // Check priority
    uint8_t priority = deserializeU8(&ptr);
    if (priority >= CYPHAL_PRIORITY_LEVELS) {
        return false;
    }
    
    // Skip node IDs and flags
    ptr += 4;
    
    // Check payload size
    uint32_t payload_size = deserializeU32(&ptr);
    if (payload_size > UAVCAN_MAX_PAYLOAD_SIZE) {
        return false;
    }
    
    // Check if buffer has enough data for the payload
    size_t header_size = (size_t)(ptr - buffer);
    if (buffer_size < header_size + payload_size) {
        return false;
    }
    
    return true;
}
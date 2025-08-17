#include "uavcan/uavcan_message_handler.h"

// Include libudpard for future integration
// Note: This is a placeholder for future libudpard integration
// The current implementation uses a simplified serialization format

#ifdef LIBUDPARD_AVAILABLE
#include "udpard.h"

/**
 * @brief Convert UavcanMessage to UdpardPayload for libudpard transmission
 * 
 * @param msg UAVCAN message to convert
 * @param payload Output payload structure
 * @param buffer Buffer to hold serialized data
 * @param buffer_size Size of buffer
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageToUdpardPayload(const UavcanMessage* msg, 
                                    struct UdpardPayload* payload,
                                    uint8_t* buffer, size_t buffer_size)
{
    if (msg == NULL || payload == NULL || buffer == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    size_t serialized_size;
    error_t result = uavcanMessageSerialize(msg, buffer, buffer_size, &serialized_size);
    if (result != NO_ERROR) {
        return result;
    }

    payload->size = serialized_size;
    payload->data = buffer;

    return NO_ERROR;
}

/**
 * @brief Convert UdpardPayload to UavcanMessage after libudpard reception
 * 
 * @param payload Input payload from libudpard
 * @param msg Output UAVCAN message
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageFromUdpardPayload(const struct UdpardPayload* payload,
                                      UavcanMessage* msg)
{
    if (payload == NULL || msg == NULL || payload->data == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    return uavcanMessageDeserialize((const uint8_t*)payload->data, 
                                   payload->size, msg);
}

/**
 * @brief Example function showing how to publish a message using libudpard
 * 
 * @param tx_instance Libudpard transmission instance
 * @param msg UAVCAN message to publish
 * @param deadline_usec Transmission deadline
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessagePublishWithLibudpard(struct UdpardTx* tx_instance,
                                         const UavcanMessage* msg,
                                         UdpardMicrosecond deadline_usec)
{
    if (tx_instance == NULL || msg == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    // Convert message to libudpard payload
    uint8_t buffer[UAVCAN_MAX_PAYLOAD_SIZE + 64]; // Extra space for headers
    struct UdpardPayload payload;
    
    error_t result = uavcanMessageToUdpardPayload(msg, &payload, buffer, sizeof(buffer));
    if (result != NO_ERROR) {
        return result;
    }

    // Map UAVCAN priority to libudpard priority
    enum UdpardPriority udpard_priority;
    switch (msg->priority) {
        case CYPHAL_PRIORITY_EXCEPTIONAL: udpard_priority = UdpardPriorityExceptional; break;
        case CYPHAL_PRIORITY_IMMEDIATE:    udpard_priority = UdpardPriorityImmediate; break;
        case CYPHAL_PRIORITY_FAST:         udpard_priority = UdpardPriorityFast; break;
        case CYPHAL_PRIORITY_HIGH:         udpard_priority = UdpardPriorityHigh; break;
        case CYPHAL_PRIORITY_NOMINAL:      udpard_priority = UdpardPriorityNominal; break;
        case CYPHAL_PRIORITY_LOW:          udpard_priority = UdpardPriorityLow; break;
        case CYPHAL_PRIORITY_SLOW:         udpard_priority = UdpardPrioritySlow; break;
        case CYPHAL_PRIORITY_OPTIONAL:     udpard_priority = UdpardPriorityOptional; break;
        default:                           udpard_priority = UdpardPriorityNominal; break;
    }

    // Publish using libudpard
    int32_t result_frames = udpardTxPublish(tx_instance,
                                           deadline_usec,
                                           udpard_priority,
                                           (UdpardPortID)msg->subject_id,
                                           0, // transfer_id (should be managed by caller)
                                           payload,
                                           NULL); // user_transfer_reference

    return (result_frames >= 0) ? NO_ERROR : ERROR_FAILURE;
}

#else

// Placeholder implementations when libudpard is not available
error_t uavcanMessageToUdpardPayload(const UavcanMessage* msg, 
                                    void* payload,
                                    uint8_t* buffer, size_t buffer_size)
{
    (void)msg;
    (void)payload;
    (void)buffer;
    (void)buffer_size;
    return ERROR_FAILURE; // Not implemented without libudpard
}

error_t uavcanMessageFromUdpardPayload(const void* payload,
                                      UavcanMessage* msg)
{
    (void)payload;
    (void)msg;
    return ERROR_FAILURE; // Not implemented without libudpard
}

error_t uavcanMessagePublishWithLibudpard(void* tx_instance,
                                         const UavcanMessage* msg,
                                         uint64_t deadline_usec)
{
    (void)tx_instance;
    (void)msg;
    (void)deadline_usec;
    return ERROR_FAILURE; // Not implemented without libudpard
}

#endif // LIBUDPARD_AVAILABLE
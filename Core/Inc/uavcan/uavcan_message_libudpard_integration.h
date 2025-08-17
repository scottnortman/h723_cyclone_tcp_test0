#ifndef UAVCAN_MESSAGE_LIBUDPARD_INTEGRATION_H
#define UAVCAN_MESSAGE_LIBUDPARD_INTEGRATION_H

#include "uavcan_message_handler.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Convert UavcanMessage to libudpard payload format
 * 
 * @param msg UAVCAN message to convert
 * @param payload Output payload structure (libudpard format)
 * @param buffer Buffer to hold serialized data
 * @param buffer_size Size of buffer
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageToUdpardPayload(const UavcanMessage* msg, 
                                    void* payload,
                                    uint8_t* buffer, size_t buffer_size);

/**
 * @brief Convert libudpard payload to UavcanMessage
 * 
 * @param payload Input payload from libudpard
 * @param msg Output UAVCAN message
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessageFromUdpardPayload(const void* payload,
                                      UavcanMessage* msg);

/**
 * @brief Publish a message using libudpard transmission pipeline
 * 
 * @param tx_instance Libudpard transmission instance
 * @param msg UAVCAN message to publish
 * @param deadline_usec Transmission deadline in microseconds
 * @return error_t NO_ERROR on success, error code otherwise
 */
error_t uavcanMessagePublishWithLibudpard(void* tx_instance,
                                         const UavcanMessage* msg,
                                         uint64_t deadline_usec);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_MESSAGE_LIBUDPARD_INTEGRATION_H
#ifndef UAVCAN_LIBUDPARD_INTEGRATION_H
#define UAVCAN_LIBUDPARD_INTEGRATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// libudpard includes
#include "udpard.h"

// UAVCAN includes
#include "uavcan_types.h"
#include "uavcan_udp_transport.h"

#ifdef __cplusplus
extern "C" {
#endif

// Integration configuration
#define UAVCAN_LIBUDPARD_MAX_TRANSFER_ID 0xFFFFFFFFUL
#define UAVCAN_LIBUDPARD_DEFAULT_MTU 1500
#define UAVCAN_LIBUDPARD_TX_QUEUE_CAPACITY 16

// Libudpard Integration Context
typedef struct {
    UdpardInstance* udpard_instance;       // Reference to libudpard instance
    UavcanUdpTransport* udp_transport;     // Reference to UDP transport
    uint64_t transfer_id_counter;          // Transfer ID counter for outgoing messages
    bool initialized;                      // Initialization status
} UavcanLibudpardIntegration;

// Function prototypes

/**
 * @brief Initialize the libudpard integration
 * @param integration Pointer to integration context
 * @param udp_transport Pointer to initialized UDP transport
 * @param node_id Local node ID
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanLibudpardIntegrationInit(UavcanLibudpardIntegration* integration,
                                           UavcanUdpTransport* udp_transport,
                                           uint8_t node_id);

/**
 * @brief Deinitialize the libudpard integration
 * @param integration Pointer to integration context
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanLibudpardIntegrationDeinit(UavcanLibudpardIntegration* integration);

/**
 * @brief Publish a message using libudpard
 * @param integration Pointer to integration context
 * @param msg UAVCAN message to publish
 * @param deadline_usec Transmission deadline in microseconds
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanLibudpardPublish(UavcanLibudpardIntegration* integration,
                                   const UavcanMessage* msg,
                                   uint64_t deadline_usec);

/**
 * @brief Send a service request using libudpard
 * @param integration Pointer to integration context
 * @param service_id Service ID
 * @param destination_node_id Destination node ID
 * @param payload Payload data
 * @param payload_size Size of payload
 * @param deadline_usec Transmission deadline in microseconds
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanLibudpardSendRequest(UavcanLibudpardIntegration* integration,
                                       uint16_t service_id,
                                       uint8_t destination_node_id,
                                       const void* payload,
                                       size_t payload_size,
                                       uint64_t deadline_usec);

/**
 * @brief Send a service response using libudpard
 * @param integration Pointer to integration context
 * @param service_id Service ID
 * @param destination_node_id Destination node ID
 * @param request_transfer_id Transfer ID from the original request
 * @param payload Payload data
 * @param payload_size Size of payload
 * @param deadline_usec Transmission deadline in microseconds
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanLibudpardSendResponse(UavcanLibudpardIntegration* integration,
                                        uint16_t service_id,
                                        uint8_t destination_node_id,
                                        uint64_t request_transfer_id,
                                        const void* payload,
                                        size_t payload_size,
                                        uint64_t deadline_usec);

/**
 * @brief Process outgoing UDP datagrams from libudpard TX queue
 * @param integration Pointer to integration context
 * @param max_datagrams Maximum number of datagrams to process
 * @return Number of datagrams processed, negative on error
 */
int32_t uavcanLibudpardProcessTxQueue(UavcanLibudpardIntegration* integration,
                                      uint32_t max_datagrams);

/**
 * @brief Process incoming UDP datagram with libudpard RX
 * @param integration Pointer to integration context
 * @param datagram Received UDP datagram
 * @param datagram_size Size of the datagram
 * @param src_addr Source IP address
 * @param timestamp_usec Reception timestamp in microseconds
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanLibudpardProcessRxDatagram(UavcanLibudpardIntegration* integration,
                                             const void* datagram,
                                             size_t datagram_size,
                                             const IpAddr* src_addr,
                                             uint64_t timestamp_usec);

/**
 * @brief Convert UAVCAN priority to libudpard priority
 * @param uavcan_priority UAVCAN priority level (0-7)
 * @return Corresponding UdpardPriority value
 */
UdpardPriority uavcanLibudpardConvertPriority(uint8_t uavcan_priority);

/**
 * @brief Convert libudpard priority to UAVCAN priority
 * @param udpard_priority libudpard priority value
 * @return Corresponding UAVCAN priority level (0-7)
 */
uint8_t uavcanLibudpardConvertPriorityFromUdpard(UdpardPriority udpard_priority);

/**
 * @brief Get current microsecond timestamp
 * @return Current timestamp in microseconds
 */
uint64_t uavcanLibudpardGetTimestampUsec(void);

/**
 * @brief Convert UAVCAN message to libudpard payload
 * @param msg UAVCAN message
 * @param payload Output libudpard payload
 * @param buffer Buffer for payload data
 * @param buffer_size Size of buffer
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanLibudpardMessageToPayload(const UavcanMessage* msg,
                                            UdpardPayload* payload,
                                            uint8_t* buffer,
                                            size_t buffer_size);

/**
 * @brief Convert libudpard transfer to UAVCAN message
 * @param transfer libudpard transfer
 * @param msg Output UAVCAN message
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanLibudpardTransferToMessage(const UdpardRxTransfer* transfer,
                                             UavcanMessage* msg);

/**
 * @brief Check if the integration is ready for operations
 * @param integration Pointer to integration context
 * @return true if ready, false otherwise
 */
bool uavcanLibudpardIntegrationIsReady(const UavcanLibudpardIntegration* integration);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_LIBUDPARD_INTEGRATION_H
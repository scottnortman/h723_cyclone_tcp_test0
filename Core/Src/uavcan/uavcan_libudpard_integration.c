#include "uavcan/uavcan_libudpard_integration.h"
#include "uavcan/uavcan_common.h"
#include "uavcan/uavcan_message_handler.h"

// Standard includes
#include <string.h>
#include <stdio.h>

// FreeRTOS includes
#include "FreeRTOS.h"
#include "task.h"

// CycloneTCP includes for IP address conversion
#include "ipv4/ipv4.h"

UavcanError uavcanLibudpardIntegrationInit(UavcanLibudpardIntegration* integration,
                                           UavcanUdpTransport* udp_transport,
                                           uint8_t node_id)
{
    if (!integration || !udp_transport) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!uavcanUdpTransportIsReady(udp_transport)) {
        return UAVCAN_ERROR_NETWORK_UNAVAILABLE;
    }

    // Initialize structure
    memset(integration, 0, sizeof(UavcanLibudpardIntegration));
    integration->udp_transport = udp_transport;
    integration->udpard_instance = uavcanUdpTransportGetUdpardInstance(udp_transport);
    
    if (!integration->udpard_instance) {
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Configure libudpard instance
    integration->udpard_instance->node_id = node_id;
    integration->udpard_instance->tx.mtu = UAVCAN_LIBUDPARD_DEFAULT_MTU;
    integration->udpard_instance->tx.queue_capacity = UAVCAN_LIBUDPARD_TX_QUEUE_CAPACITY;

    // Initialize transfer ID counter
    integration->transfer_id_counter = 0;
    integration->initialized = true;

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanLibudpardIntegrationDeinit(UavcanLibudpardIntegration* integration)
{
    if (!integration || !integration->initialized) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Clean up any pending transfers in libudpard
    // Note: libudpard will clean up automatically when the instance is destroyed
    
    integration->initialized = false;
    integration->udpard_instance = NULL;
    integration->udp_transport = NULL;
    integration->transfer_id_counter = 0;

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanLibudpardPublish(UavcanLibudpardIntegration* integration,
                                   const UavcanMessage* msg,
                                   uint64_t deadline_usec)
{
    if (!integration || !integration->initialized || !msg) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (!msg->payload || msg->payload_size == 0) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Convert UAVCAN message to libudpard payload
    uint8_t buffer[UAVCAN_MAX_PAYLOAD_SIZE];
    UdpardPayload payload;
    
    UavcanError result = uavcanLibudpardMessageToPayload(msg, &payload, buffer, sizeof(buffer));
    if (result != UAVCAN_ERROR_NONE) {
        return result;
    }

    // Convert priority
    UdpardPriority priority = uavcanLibudpardConvertPriority(msg->priority);

    // Get next transfer ID
    uint64_t transfer_id = integration->transfer_id_counter++;
    if (integration->transfer_id_counter > UAVCAN_LIBUDPARD_MAX_TRANSFER_ID) {
        integration->transfer_id_counter = 0;
    }

    // Publish using libudpard
    int32_t frames = udpardTxPublish(&integration->udpard_instance->tx,
                                     deadline_usec,
                                     priority,
                                     (UdpardPortID)msg->subject_id,
                                     transfer_id,
                                     &payload,
                                     NULL); // user_transfer_reference

    if (frames < 0) {
        return UAVCAN_ERROR_SEND_FAILED;
    }

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanLibudpardSendRequest(UavcanLibudpardIntegration* integration,
                                       uint16_t service_id,
                                       uint8_t destination_node_id,
                                       const void* payload,
                                       size_t payload_size,
                                       uint64_t deadline_usec)
{
    if (!integration || !integration->initialized || !payload) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (payload_size == 0 || payload_size > UAVCAN_MAX_PAYLOAD_SIZE) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Create libudpard payload
    UdpardPayload udpard_payload = {
        .size = payload_size,
        .data = payload
    };

    // Get next transfer ID
    uint64_t transfer_id = integration->transfer_id_counter++;
    if (integration->transfer_id_counter > UAVCAN_LIBUDPARD_MAX_TRANSFER_ID) {
        integration->transfer_id_counter = 0;
    }

    // Send request using libudpard
    int32_t frames = udpardTxRequest(&integration->udpard_instance->tx,
                                     deadline_usec,
                                     UdpardPriorityNominal, // Default priority for services
                                     (UdpardPortID)service_id,
                                     destination_node_id,
                                     transfer_id,
                                     &udpard_payload,
                                     NULL); // user_transfer_reference

    if (frames < 0) {
        return UAVCAN_ERROR_SEND_FAILED;
    }

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanLibudpardSendResponse(UavcanLibudpardIntegration* integration,
                                        uint16_t service_id,
                                        uint8_t destination_node_id,
                                        uint64_t request_transfer_id,
                                        const void* payload,
                                        size_t payload_size,
                                        uint64_t deadline_usec)
{
    if (!integration || !integration->initialized || !payload) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (payload_size == 0 || payload_size > UAVCAN_MAX_PAYLOAD_SIZE) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Create libudpard payload
    UdpardPayload udpard_payload = {
        .size = payload_size,
        .data = payload
    };

    // Send response using libudpard
    int32_t frames = udpardTxRespond(&integration->udpard_instance->tx,
                                     deadline_usec,
                                     UdpardPriorityNominal, // Default priority for services
                                     (UdpardPortID)service_id,
                                     destination_node_id,
                                     request_transfer_id,
                                     &udpard_payload,
                                     NULL); // user_transfer_reference

    if (frames < 0) {
        return UAVCAN_ERROR_SEND_FAILED;
    }

    return UAVCAN_ERROR_NONE;
}

int32_t uavcanLibudpardProcessTxQueue(UavcanLibudpardIntegration* integration,
                                      uint32_t max_datagrams)
{
    if (!integration || !integration->initialized) {
        return -1;
    }

    int32_t processed = 0;
    
    for (uint32_t i = 0; i < max_datagrams; i++) {
        // Pop next datagram from libudpard TX queue
        UdpardTxItem* tx_item = udpardTxPop(&integration->udpard_instance->tx,
                                            uavcanLibudpardGetTimestampUsec());
        
        if (!tx_item) {
            break; // No more datagrams to send
        }

        // Send the datagram through UDP transport
        UavcanError result = uavcanUdpTransportSend(integration->udp_transport,
                                                    tx_item->datagram_payload.data,
                                                    tx_item->datagram_payload.size,
                                                    NULL, // Use multicast
                                                    0);   // Use default port

        // Free the TX item
        integration->udpard_instance->memory.deallocate(integration->udpard_instance->memory.user_reference, tx_item);

        if (result == UAVCAN_ERROR_NONE) {
            processed++;
        } else {
            // Log error but continue processing
            break;
        }
    }

    return processed;
}

UavcanError uavcanLibudpardProcessRxDatagram(UavcanLibudpardIntegration* integration,
                                             const void* datagram,
                                             size_t datagram_size,
                                             const IpAddr* src_addr,
                                             uint64_t timestamp_usec)
{
    if (!integration || !integration->initialized || !datagram || !src_addr) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (datagram_size == 0) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Convert IP address to libudpard format
    // For now, we'll use a simplified approach and extract the last octet as session specifier
    uint32_t ip_addr = src_addr->ipv4Addr;
    uint8_t session_specifier = (uint8_t)(ip_addr & 0xFF);

    // Create libudpard datagram
    UdpardRxDatagram rx_datagram = {
        .payload = {
            .size = datagram_size,
            .data = datagram
        },
        .timestamp_usec = timestamp_usec,
        .session_specifier = session_specifier
    };

    // Process with libudpard RX
    UdpardRxTransfer transfer;
    int8_t result = udpardRxAccept(&integration->udpard_instance->rx,
                                   &rx_datagram,
                                   0, // redundant_interface_index
                                   &transfer,
                                   NULL); // user_transfer_reference

    if (result == 1) {
        // Complete transfer received
        // The transfer can now be processed by the application
        // For now, we'll just indicate success
        
        // Note: The application should handle the transfer and then free it
        // using integration->udpard_instance->memory.deallocate()
    } else if (result == 0) {
        // Incomplete transfer, waiting for more fragments
        // This is normal and not an error
    } else {
        // Error occurred
        return UAVCAN_ERROR_RECEIVE_FAILED;
    }

    return UAVCAN_ERROR_NONE;
}

UdpardPriority uavcanLibudpardConvertPriority(uint8_t uavcan_priority)
{
    switch (uavcan_priority) {
        case CYPHAL_PRIORITY_EXCEPTIONAL: return UdpardPriorityExceptional;
        case CYPHAL_PRIORITY_IMMEDIATE:    return UdpardPriorityImmediate;
        case CYPHAL_PRIORITY_FAST:         return UdpardPriorityFast;
        case CYPHAL_PRIORITY_HIGH:         return UdpardPriorityHigh;
        case CYPHAL_PRIORITY_NOMINAL:      return UdpardPriorityNominal;
        case CYPHAL_PRIORITY_LOW:          return UdpardPriorityLow;
        case CYPHAL_PRIORITY_SLOW:         return UdpardPrioritySlow;
        case CYPHAL_PRIORITY_OPTIONAL:     return UdpardPriorityOptional;
        default:                           return UdpardPriorityNominal;
    }
}

uint8_t uavcanLibudpardConvertPriorityFromUdpard(UdpardPriority udpard_priority)
{
    switch (udpard_priority) {
        case UdpardPriorityExceptional: return CYPHAL_PRIORITY_EXCEPTIONAL;
        case UdpardPriorityImmediate:    return CYPHAL_PRIORITY_IMMEDIATE;
        case UdpardPriorityFast:         return CYPHAL_PRIORITY_FAST;
        case UdpardPriorityHigh:         return CYPHAL_PRIORITY_HIGH;
        case UdpardPriorityNominal:      return CYPHAL_PRIORITY_NOMINAL;
        case UdpardPriorityLow:          return CYPHAL_PRIORITY_LOW;
        case UdpardPrioritySlow:         return CYPHAL_PRIORITY_SLOW;
        case UdpardPriorityOptional:     return CYPHAL_PRIORITY_OPTIONAL;
        default:                         return CYPHAL_PRIORITY_NOMINAL;
    }
}

uint64_t uavcanLibudpardGetTimestampUsec(void)
{
    // Get FreeRTOS tick count and convert to microseconds
    TickType_t ticks = xTaskGetTickCount();
    uint64_t usec = (uint64_t)ticks * (1000000ULL / configTICK_RATE_HZ);
    return usec;
}

UavcanError uavcanLibudpardMessageToPayload(const UavcanMessage* msg,
                                            UdpardPayload* payload,
                                            uint8_t* buffer,
                                            size_t buffer_size)
{
    if (!msg || !payload || !buffer) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (msg->payload_size > buffer_size) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // For now, we'll use a simple copy approach
    // In a full implementation, this would involve proper DSDL serialization
    memcpy(buffer, msg->payload, msg->payload_size);
    
    payload->size = msg->payload_size;
    payload->data = buffer;

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanLibudpardTransferToMessage(const UdpardRxTransfer* transfer,
                                             UavcanMessage* msg)
{
    if (!transfer || !msg) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize message structure
    memset(msg, 0, sizeof(UavcanMessage));

    // Extract information from libudpard transfer
    msg->subject_id = (uint32_t)transfer->port_id;
    msg->priority = uavcanLibudpardConvertPriorityFromUdpard(transfer->priority);
    msg->source_node_id = transfer->source_node_id;
    msg->timestamp_usec = transfer->timestamp_usec;
    
    // Handle payload
    if (transfer->payload.size > 0 && transfer->payload.size <= UAVCAN_MAX_PAYLOAD_SIZE) {
        msg->payload_size = transfer->payload.size;
        msg->payload = (uint8_t*)transfer->payload.data;
    } else {
        msg->payload_size = 0;
        msg->payload = NULL;
    }

    // Set message type based on transfer metadata
    msg->is_service_request = (transfer->metadata.transfer_kind == UdpardTransferKindRequest);
    msg->is_anonymous = (transfer->source_node_id == UDPARD_NODE_ID_UNSET);

    return UAVCAN_ERROR_NONE;
}

bool uavcanLibudpardIntegrationIsReady(const UavcanLibudpardIntegration* integration)
{
    return integration && 
           integration->initialized && 
           integration->udpard_instance && 
           integration->udp_transport &&
           uavcanUdpTransportIsReady(integration->udp_transport);
}
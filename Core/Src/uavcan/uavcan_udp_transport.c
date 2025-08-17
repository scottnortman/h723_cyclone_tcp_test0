#include "uavcan/uavcan_udp_transport.h"
#include "uavcan/uavcan_common.h"

// Standard includes
#include <string.h>
#include <stdio.h>

// CycloneTCP includes
#include "core/socket.h"
#include "ipv4/ipv4_multicast.h"

// Memory allocator for libudpard (using FreeRTOS heap)
static void* udpard_malloc(size_t size)
{
    return pvPortMalloc(size);
}

static void udpard_free(void* ptr)
{
    vPortFree(ptr);
}

UavcanError uavcanUdpTransportInit(UavcanUdpTransport* transport, 
                                   NetInterface* net_interface,
                                   uint16_t port, 
                                   const char* multicast_addr)
{
    if (!transport || !net_interface || !multicast_addr) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Initialize structure
    memset(transport, 0, sizeof(UavcanUdpTransport));
    transport->net_interface = net_interface;
    transport->port = port;

    // Parse multicast address
    error_t err = ipv4StringToAddr(multicast_addr, &transport->multicast_addr);
    if (err != NO_ERROR) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Create socket mutex
    transport->socket_mutex = xSemaphoreCreateMutex();
    if (!transport->socket_mutex) {
        return UAVCAN_ERROR_MEMORY_ALLOCATION;
    }

    // Create UDP socket
    transport->udp_socket = socketOpen(SOCKET_TYPE_DGRAM, IP_PROTOCOL_UDP);
    if (!transport->udp_socket) {
        vSemaphoreDelete(transport->socket_mutex);
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Bind socket to the specified port
    err = socketBind(transport->udp_socket, &IP_ADDR_ANY, port);
    if (err != NO_ERROR) {
        socketClose(transport->udp_socket);
        vSemaphoreDelete(transport->socket_mutex);
        return UAVCAN_ERROR_INIT_FAILED;
    }

    // Set socket to non-blocking mode for receive operations
    socketSetTimeout(transport->udp_socket, UAVCAN_UDP_TRANSPORT_SOCKET_TIMEOUT_MS);

    // Join the UAVCAN multicast group
    UavcanError result = uavcanUdpTransportJoinMulticast(transport, &transport->multicast_addr);
    if (result != UAVCAN_ERROR_NONE) {
        socketClose(transport->udp_socket);
        vSemaphoreDelete(transport->socket_mutex);
        return result;
    }

    // Initialize libudpard instance
    UdpardInstance* udpard = &transport->udpard_instance;
    memset(udpard, 0, sizeof(UdpardInstance));
    
    // Configure libudpard memory allocator
    udpard->memory.allocate = udpard_malloc;
    udpard->memory.deallocate = udpard_free;
    udpard->memory.user_reference = NULL;

    // Set default MTU (can be configured later if needed)
    udpard->tx.mtu = UDPARD_MTU_DEFAULT;

    transport->initialized = true;
    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanUdpTransportDeinit(UavcanUdpTransport* transport)
{
    if (!transport || !transport->initialized) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Take mutex to ensure no concurrent operations
    if (xSemaphoreTake(transport->socket_mutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        // Leave multicast group
        uavcanUdpTransportLeaveMulticast(transport, &transport->multicast_addr);

        // Close socket
        if (transport->udp_socket) {
            socketClose(transport->udp_socket);
            transport->udp_socket = NULL;
        }

        // Clean up libudpard resources
        // Note: libudpard doesn't have explicit cleanup, but we should free any allocated memory
        // This would typically be done by the memory allocator when the instance is destroyed

        transport->initialized = false;
        
        xSemaphoreGive(transport->socket_mutex);
    }

    // Delete mutex
    if (transport->socket_mutex) {
        vSemaphoreDelete(transport->socket_mutex);
        transport->socket_mutex = NULL;
    }

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanUdpTransportSend(UavcanUdpTransport* transport, 
                                   const void* data, 
                                   size_t size,
                                   const IpAddr* dest_addr,
                                   uint16_t dest_port)
{
    if (!transport || !transport->initialized || !data || size == 0) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    if (size > UAVCAN_UDP_TRANSPORT_MAX_PAYLOAD_SIZE) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Use default values if not specified
    const IpAddr* target_addr = dest_addr ? dest_addr : &transport->multicast_addr;
    uint16_t target_port = dest_port ? dest_port : transport->port;

    // Take mutex for thread safety
    if (xSemaphoreTake(transport->socket_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    size_t sent = 0;
    error_t err = socketSendTo(transport->udp_socket, target_addr, target_port, 
                               data, size, &sent, 0);

    xSemaphoreGive(transport->socket_mutex);

    if (err != NO_ERROR) {
        return UAVCAN_ERROR_SEND_FAILED;
    }

    if (sent != size) {
        return UAVCAN_ERROR_SEND_FAILED;
    }

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanUdpTransportReceive(UavcanUdpTransport* transport, 
                                      void* buffer, 
                                      size_t buffer_size,
                                      size_t* received_size,
                                      IpAddr* src_addr,
                                      uint16_t* src_port,
                                      uint32_t timeout_ms)
{
    if (!transport || !transport->initialized || !buffer || !received_size) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    *received_size = 0;

    // Take mutex for thread safety
    if (xSemaphoreTake(transport->socket_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    // Temporarily set socket timeout
    uint32_t original_timeout = transport->udp_socket->timeout;
    socketSetTimeout(transport->udp_socket, timeout_ms);

    IpAddr temp_src_addr;
    uint16_t temp_src_port;
    size_t received = 0;

    error_t err = socketReceiveFrom(transport->udp_socket, 
                                    &temp_src_addr, &temp_src_port,
                                    buffer, buffer_size, &received, 0);

    // Restore original timeout
    socketSetTimeout(transport->udp_socket, original_timeout);

    xSemaphoreGive(transport->socket_mutex);

    if (err == ERROR_TIMEOUT) {
        return UAVCAN_ERROR_TIMEOUT;
    } else if (err != NO_ERROR) {
        return UAVCAN_ERROR_RECEIVE_FAILED;
    }

    *received_size = received;
    
    if (src_addr) {
        *src_addr = temp_src_addr;
    }
    
    if (src_port) {
        *src_port = temp_src_port;
    }

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanUdpTransportJoinMulticast(UavcanUdpTransport* transport, 
                                            const IpAddr* multicast_addr)
{
    if (!transport || !transport->initialized || !multicast_addr) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Take mutex for thread safety
    if (xSemaphoreTake(transport->socket_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    error_t err = socketJoinMulticastGroup(transport->udp_socket, multicast_addr);

    xSemaphoreGive(transport->socket_mutex);

    if (err != NO_ERROR) {
        return UAVCAN_ERROR_NETWORK_UNAVAILABLE;
    }

    return UAVCAN_ERROR_NONE;
}

UavcanError uavcanUdpTransportLeaveMulticast(UavcanUdpTransport* transport, 
                                             const IpAddr* multicast_addr)
{
    if (!transport || !transport->initialized || !multicast_addr) {
        return UAVCAN_ERROR_INVALID_PARAMETER;
    }

    // Take mutex for thread safety
    if (xSemaphoreTake(transport->socket_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return UAVCAN_ERROR_TIMEOUT;
    }

    error_t err = socketLeaveMulticastGroup(transport->udp_socket, multicast_addr);

    xSemaphoreGive(transport->socket_mutex);

    if (err != NO_ERROR) {
        return UAVCAN_ERROR_NETWORK_UNAVAILABLE;
    }

    return UAVCAN_ERROR_NONE;
}

bool uavcanUdpTransportIsReady(const UavcanUdpTransport* transport)
{
    return transport && transport->initialized && transport->udp_socket;
}

Socket* uavcanUdpTransportGetSocket(const UavcanUdpTransport* transport)
{
    if (!transport || !transport->initialized) {
        return NULL;
    }
    return transport->udp_socket;
}

UdpardInstance* uavcanUdpTransportGetUdpardInstance(UavcanUdpTransport* transport)
{
    if (!transport || !transport->initialized) {
        return NULL;
    }
    return &transport->udpard_instance;
}
#ifndef UAVCAN_UDP_TRANSPORT_H
#define UAVCAN_UDP_TRANSPORT_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// CycloneTCP includes
#include "core/net.h"
#include "core/socket.h"
#include "core/udp.h"
#include "ipv4/ipv4.h"
#include "error.h"

// FreeRTOS includes
#include "FreeRTOS.h"
#include "semphr.h"

// libudpard include
#include "udpard.h"

// UAVCAN includes
#include "uavcan_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// UDP Transport Configuration
#define UAVCAN_UDP_TRANSPORT_MAX_PAYLOAD_SIZE 1024
#define UAVCAN_UDP_TRANSPORT_SOCKET_TIMEOUT_MS 100

// UDP Transport Structure
typedef struct {
    Socket* udp_socket;                    // CycloneTCP socket handle
    IpAddr multicast_addr;                 // UAVCAN multicast IP address
    uint16_t port;                         // UDP port number
    SemaphoreHandle_t socket_mutex;        // Mutex for thread-safe socket access
    UdpardInstance udpard_instance;        // libudpard instance
    bool initialized;                      // Initialization status
    NetInterface* net_interface;           // Network interface reference
} UavcanUdpTransport;

// Function prototypes

/**
 * @brief Initialize the UAVCAN UDP transport layer
 * @param transport Pointer to transport structure
 * @param net_interface Network interface to use
 * @param port UDP port number (typically UAVCAN_UDP_PORT_DEFAULT)
 * @param multicast_addr Multicast address string (typically UAVCAN_MULTICAST_ADDR)
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanUdpTransportInit(UavcanUdpTransport* transport, 
                                   NetInterface* net_interface,
                                   uint16_t port, 
                                   const char* multicast_addr);

/**
 * @brief Deinitialize the UAVCAN UDP transport layer
 * @param transport Pointer to transport structure
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanUdpTransportDeinit(UavcanUdpTransport* transport);

/**
 * @brief Send data through the UDP transport
 * @param transport Pointer to transport structure
 * @param data Pointer to data to send
 * @param size Size of data in bytes
 * @param dest_addr Destination IP address (NULL for multicast)
 * @param dest_port Destination port (0 to use transport's port)
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanUdpTransportSend(UavcanUdpTransport* transport, 
                                   const void* data, 
                                   size_t size,
                                   const IpAddr* dest_addr,
                                   uint16_t dest_port);

/**
 * @brief Receive data from the UDP transport
 * @param transport Pointer to transport structure
 * @param buffer Buffer to store received data
 * @param buffer_size Size of the buffer
 * @param received_size Pointer to store actual received size
 * @param src_addr Pointer to store source IP address (optional)
 * @param src_port Pointer to store source port (optional)
 * @param timeout_ms Timeout in milliseconds
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanUdpTransportReceive(UavcanUdpTransport* transport, 
                                      void* buffer, 
                                      size_t buffer_size,
                                      size_t* received_size,
                                      IpAddr* src_addr,
                                      uint16_t* src_port,
                                      uint32_t timeout_ms);

/**
 * @brief Join a multicast group
 * @param transport Pointer to transport structure
 * @param multicast_addr Multicast address to join
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanUdpTransportJoinMulticast(UavcanUdpTransport* transport, 
                                            const IpAddr* multicast_addr);

/**
 * @brief Leave a multicast group
 * @param transport Pointer to transport structure
 * @param multicast_addr Multicast address to leave
 * @return UAVCAN_ERROR_NONE on success, error code otherwise
 */
UavcanError uavcanUdpTransportLeaveMulticast(UavcanUdpTransport* transport, 
                                             const IpAddr* multicast_addr);

/**
 * @brief Check if the transport is ready for operations
 * @param transport Pointer to transport structure
 * @return true if ready, false otherwise
 */
bool uavcanUdpTransportIsReady(const UavcanUdpTransport* transport);

/**
 * @brief Get the socket handle for external operations
 * @param transport Pointer to transport structure
 * @return Socket handle or NULL if not initialized
 */
Socket* uavcanUdpTransportGetSocket(const UavcanUdpTransport* transport);

/**
 * @brief Get the libudpard instance for external operations
 * @param transport Pointer to transport structure
 * @return Pointer to UdpardInstance or NULL if not initialized
 */
UdpardInstance* uavcanUdpTransportGetUdpardInstance(UavcanUdpTransport* transport);

#ifdef __cplusplus
}
#endif

#endif // UAVCAN_UDP_TRANSPORT_H
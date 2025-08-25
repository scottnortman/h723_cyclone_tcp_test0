/**
 * @file uavcan_transport.c
 * @brief UAVCAN UDP Transport Layer implementation
 * 
 * This file implements the UAVCAN UDP transport operations using CycloneTCP
 * including socket management, multicast group handling, and thread-safe operations.
 */

/* Includes ------------------------------------------------------------------*/
#include "uavcan/uavcan_transport.h"
#include "uavcan/uavcan_types.h"

// CycloneTCP includes
#include "core/net.h"
#include "core/socket.h"
#include "core/udp.h"
#include "ipv4/ipv4.h"
#include "ipv4/ipv4_multicast.h"
#include "error.h"

#include <string.h>

/* Private defines -----------------------------------------------------------*/
#define UAVCAN_TRANSPORT_RECV_TIMEOUT_MS    100
#define UAVCAN_TRANSPORT_SEND_TIMEOUT_MS    50

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/
static error_t uavcanTransportConfigureSocket(UavcanTransport* transport);
static uint32_t uavcanTransportIpv4FromBytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

/* Exported functions --------------------------------------------------------*/

/**
 * @brief Initialize UAVCAN transport
 * @param transport Pointer to transport structure
 * @param interface Pointer to network interface
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportInit(UavcanTransport* transport, NetInterface* interface)
{
    error_t error;
    
    // Validate parameters
    if (transport == NULL || interface == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Initialize structure
    memset(transport, 0, sizeof(UavcanTransport));
    transport->interface = interface;
    transport->local_port = UAVCAN_UDP_PORT;
    transport->multicast_enabled = false;
    transport->initialized = false;
    
    // Create mutex for thread-safe socket operations
    if (!osCreateMutex(&transport->socket_mutex)) {
        return UAVCAN_ERROR_INIT_FAILED;
    }
    
    // Create UDP socket
    transport->socket = socketOpen(SOCKET_TYPE_DGRAM, IP_PROTOCOL_UDP);
    if (transport->socket == NULL) {
        osDeleteMutex(&transport->socket_mutex);
        return UAVCAN_ERROR_SOCKET_ERROR;
    }
    
    // Configure socket
    error = uavcanTransportConfigureSocket(transport);
    if (error != NO_ERROR) {
        socketClose(transport->socket);
        osDeleteMutex(&transport->socket_mutex);
        return UAVCAN_ERROR_SOCKET_ERROR;
    }
    
    // Bind socket to UAVCAN port
    error = socketBind(transport->socket, &IP_ADDR_ANY, transport->local_port);
    if (error != NO_ERROR) {
        socketClose(transport->socket);
        osDeleteMutex(&transport->socket_mutex);
        return UAVCAN_ERROR_SOCKET_ERROR;
    }
    
    transport->initialized = true;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Deinitialize UAVCAN transport
 * @param transport Pointer to transport structure
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportDeinit(UavcanTransport* transport)
{
    if (transport == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!transport->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Close socket
    if (transport->socket != NULL) {
        socketClose(transport->socket);
        transport->socket = NULL;
    }
    
    // Delete mutex
    osDeleteMutex(&transport->socket_mutex);
    
    transport->initialized = false;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Send data via UDP transport
 * @param transport Pointer to transport structure
 * @param endpoint Pointer to destination endpoint
 * @param data Pointer to data to send
 * @param size Size of data in bytes
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportSend(UavcanTransport* transport, 
                               const UavcanUdpEndpoint* endpoint,
                               const void* data, 
                               size_t size)
{
    error_t error;
    size_t written;
    IpAddr dest_addr;
    
    // Validate parameters
    if (transport == NULL || endpoint == NULL || data == NULL || size == 0) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!transport->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Convert IP address
    dest_addr.length = sizeof(Ipv4Addr);
    dest_addr.ipv4Addr = htonl(endpoint->ip_address);
    
    // Thread-safe socket operation
    osAcquireMutex(&transport->socket_mutex);
    
    // Send data
    error = socketSendTo(transport->socket, &dest_addr, endpoint->udp_port,
                        data, size, &written, 0);
    
    osReleaseMutex(&transport->socket_mutex);
    
    if (error != NO_ERROR) {
        return UAVCAN_ERROR_NETWORK_ERROR;
    }
    
    if (written != size) {
        return UAVCAN_ERROR_NETWORK_ERROR;
    }
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Receive data via UDP transport
 * @param transport Pointer to transport structure
 * @param buffer Pointer to receive buffer
 * @param buffer_size Size of receive buffer
 * @param received_size Pointer to store actual received size
 * @param timeout Timeout in milliseconds
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportReceive(UavcanTransport* transport,
                                  void* buffer,
                                  size_t buffer_size,
                                  size_t* received_size,
                                  systime_t timeout)
{
    error_t error;
    IpAddr src_addr;
    uint16_t src_port;
    
    // Validate parameters
    if (transport == NULL || buffer == NULL || received_size == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!transport->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    *received_size = 0;
    
    // Thread-safe socket operation
    osAcquireMutex(&transport->socket_mutex);
    
    // Receive data with timeout
    error = socketReceiveFrom(transport->socket, &src_addr, &src_port,
                             buffer, buffer_size, received_size, timeout);
    
    osReleaseMutex(&transport->socket_mutex);
    
    if (error == ERROR_TIMEOUT) {
        return UAVCAN_ERROR_TIMEOUT;
    } else if (error != NO_ERROR) {
        return UAVCAN_ERROR_NETWORK_ERROR;
    }
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Join multicast group
 * @param transport Pointer to transport structure
 * @param multicast_addr Multicast address to join
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportJoinMulticast(UavcanTransport* transport, 
                                        uint32_t multicast_addr)
{
    error_t error;
    IpAddr group_addr;
    
    // Validate parameters
    if (transport == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!transport->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Convert multicast address
    group_addr.length = sizeof(Ipv4Addr);
    group_addr.ipv4Addr = htonl(multicast_addr);
    
    // Thread-safe socket operation
    osAcquireMutex(&transport->socket_mutex);
    
    // Join multicast group
    error = socketJoinMulticastGroup(transport->socket, &group_addr);
    
    osReleaseMutex(&transport->socket_mutex);
    
    if (error != NO_ERROR) {
        return UAVCAN_ERROR_NETWORK_ERROR;
    }
    
    transport->multicast_enabled = true;
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Leave multicast group
 * @param transport Pointer to transport structure
 * @param multicast_addr Multicast address to leave
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportLeaveMulticast(UavcanTransport* transport, 
                                         uint32_t multicast_addr)
{
    error_t error;
    IpAddr group_addr;
    
    // Validate parameters
    if (transport == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!transport->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Convert multicast address
    group_addr.length = sizeof(Ipv4Addr);
    group_addr.ipv4Addr = htonl(multicast_addr);
    
    // Thread-safe socket operation
    osAcquireMutex(&transport->socket_mutex);
    
    // Leave multicast group
    error = socketLeaveMulticastGroup(transport->socket, &group_addr);
    
    osReleaseMutex(&transport->socket_mutex);
    
    if (error != NO_ERROR) {
        return UAVCAN_ERROR_NETWORK_ERROR;
    }
    
    return UAVCAN_ERROR_NONE;
}

/**
 * @brief Create subject UDP endpoint
 * @param subject_id Subject ID
 * @param endpoint Pointer to endpoint structure to fill
 */
void uavcanTransportMakeSubjectEndpoint(UdpardPortID subject_id, 
                                       UavcanUdpEndpoint* endpoint)
{
    if (endpoint == NULL) {
        return;
    }
    
    // Calculate multicast address for subject
    endpoint->ip_address = UAVCAN_SUBJECT_MULTICAST_ADDR(subject_id);
    endpoint->udp_port = UAVCAN_UDP_PORT;
}

/**
 * @brief Create service UDP endpoint
 * @param destination_node_id Destination node ID
 * @param endpoint Pointer to endpoint structure to fill
 */
void uavcanTransportMakeServiceEndpoint(UdpardNodeID destination_node_id, 
                                       UavcanUdpEndpoint* endpoint)
{
    if (endpoint == NULL) {
        return;
    }
    
    // Calculate multicast address for service
    endpoint->ip_address = UAVCAN_SERVICE_MULTICAST_ADDR(destination_node_id);
    endpoint->udp_port = UAVCAN_UDP_PORT;
}

/**
 * @brief Check if transport is initialized
 * @param transport Pointer to transport structure
 * @retval bool True if initialized, false otherwise
 */
bool uavcanTransportIsInitialized(const UavcanTransport* transport)
{
    if (transport == NULL) {
        return false;
    }
    
    return transport->initialized;
}

/* Private functions ---------------------------------------------------------*/

/**
 * @brief Configure socket for UAVCAN operations
 * @param transport Pointer to transport structure
 * @retval error_t CycloneTCP error code
 */
static error_t uavcanTransportConfigureSocket(UavcanTransport* transport)
{
    error_t error;
    systime_t timeout;
    
    if (transport == NULL || transport->socket == NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    
    // Set receive timeout
    timeout = UAVCAN_TRANSPORT_RECV_TIMEOUT_MS;
    error = socketSetTimeout(transport->socket, timeout);
    if (error != NO_ERROR) {
        return error;
    }
    
    // Enable broadcast (for multicast support)
    error = socketEnableBroadcast(transport->socket, TRUE);
    if (error != NO_ERROR) {
        return error;
    }
    
    return NO_ERROR;
}

/**
 * @brief Convert IPv4 address from bytes to uint32_t
 * @param a First octet
 * @param b Second octet
 * @param c Third octet
 * @param d Fourth octet
 * @retval uint32_t IPv4 address in host byte order
 */
static uint32_t uavcanTransportIpv4FromBytes(uint8_t a, uint8_t b, uint8_t c, uint8_t d)
{
    return ((uint32_t)a << 24) | ((uint32_t)b << 16) | ((uint32_t)c << 8) | (uint32_t)d;
}

/* Extended multicast management functions -----------------------------------*/

/**
 * @brief Join UAVCAN subject multicast group
 * @param transport Pointer to transport structure
 * @param subject_id Subject ID to subscribe to
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportJoinSubjectMulticast(UavcanTransport* transport, 
                                               UdpardPortID subject_id)
{
    uint32_t multicast_addr;
    
    if (transport == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Calculate subject multicast address
    multicast_addr = UAVCAN_SUBJECT_MULTICAST_ADDR(subject_id);
    
    return uavcanTransportJoinMulticast(transport, multicast_addr);
}

/**
 * @brief Leave UAVCAN subject multicast group
 * @param transport Pointer to transport structure
 * @param subject_id Subject ID to unsubscribe from
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportLeaveSubjectMulticast(UavcanTransport* transport, 
                                                UdpardPortID subject_id)
{
    uint32_t multicast_addr;
    
    if (transport == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Calculate subject multicast address
    multicast_addr = UAVCAN_SUBJECT_MULTICAST_ADDR(subject_id);
    
    return uavcanTransportLeaveMulticast(transport, multicast_addr);
}

/**
 * @brief Join UAVCAN service multicast group
 * @param transport Pointer to transport structure
 * @param node_id Node ID for service communication
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportJoinServiceMulticast(UavcanTransport* transport, 
                                               UdpardNodeID node_id)
{
    uint32_t multicast_addr;
    
    if (transport == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Validate node ID
    if (!UAVCAN_IS_VALID_NODE_ID(node_id)) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Calculate service multicast address
    multicast_addr = UAVCAN_SERVICE_MULTICAST_ADDR(node_id);
    
    return uavcanTransportJoinMulticast(transport, multicast_addr);
}

/**
 * @brief Leave UAVCAN service multicast group
 * @param transport Pointer to transport structure
 * @param node_id Node ID for service communication
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportLeaveServiceMulticast(UavcanTransport* transport, 
                                                UdpardNodeID node_id)
{
    uint32_t multicast_addr;
    
    if (transport == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Validate node ID
    if (!UAVCAN_IS_VALID_NODE_ID(node_id)) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    // Calculate service multicast address
    multicast_addr = UAVCAN_SERVICE_MULTICAST_ADDR(node_id);
    
    return uavcanTransportLeaveMulticast(transport, multicast_addr);
}

/**
 * @brief Calculate UAVCAN subject multicast address
 * @param subject_id Subject ID
 * @retval uint32_t Multicast address in host byte order
 */
uint32_t uavcanTransportCalculateSubjectMulticast(UdpardPortID subject_id)
{
    return UAVCAN_SUBJECT_MULTICAST_ADDR(subject_id);
}

/**
 * @brief Calculate UAVCAN service multicast address
 * @param node_id Node ID
 * @retval uint32_t Multicast address in host byte order
 */
uint32_t uavcanTransportCalculateServiceMulticast(UdpardNodeID node_id)
{
    if (!UAVCAN_IS_VALID_NODE_ID(node_id)) {
        return 0;
    }
    
    return UAVCAN_SERVICE_MULTICAST_ADDR(node_id);
}

/**
 * @brief Validate multicast address for UAVCAN
 * @param multicast_addr Multicast address to validate
 * @retval bool True if valid UAVCAN multicast address
 */
bool uavcanTransportIsValidMulticastAddr(uint32_t multicast_addr)
{
    // Check if it's in the UAVCAN subject multicast range
    if ((multicast_addr & 0xFFFF0000UL) == (UAVCAN_SUBJECT_MULTICAST_BASE & 0xFFFF0000UL)) {
        return true;
    }
    
    // Check if it's in the UAVCAN service multicast range
    if ((multicast_addr & 0xFFFF0000UL) == (UAVCAN_SERVICE_MULTICAST_BASE & 0xFFFF0000UL)) {
        // Validate node ID part
        uint16_t node_id = multicast_addr & 0x0000FFFFUL;
        return UAVCAN_IS_VALID_NODE_ID(node_id);
    }
    
    return false;
}

/**
 * @brief Get transport statistics
 * @param transport Pointer to transport structure
 * @param stats Pointer to statistics structure to fill
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportGetStats(const UavcanTransport* transport, 
                                   UavcanTransportStats* stats)
{
    if (transport == NULL || stats == NULL) {
        return UAVCAN_ERROR_INVALID_PARAM;
    }
    
    if (!transport->initialized) {
        return UAVCAN_ERROR_NODE_NOT_INITIALIZED;
    }
    
    // Initialize stats structure
    memset(stats, 0, sizeof(UavcanTransportStats));
    
    // Fill basic information
    stats->initialized = transport->initialized;
    stats->multicast_enabled = transport->multicast_enabled;
    stats->local_port = transport->local_port;
    
    // Socket statistics would need to be tracked separately
    // For now, just indicate the socket is active
    stats->socket_active = (transport->socket != NULL);
    
    return UAVCAN_ERROR_NONE;
}
/**
 * @file uavcan_transport.h
 * @brief UAVCAN UDP Transport Layer interface
 * 
 * This file defines the interface for UAVCAN UDP transport operations
 * including socket management and multicast group handling.
 */

#ifndef UAVCAN_TRANSPORT_H
#define UAVCAN_TRANSPORT_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"
#include "core/socket.h"

/* Exported types ------------------------------------------------------------*/

/**
 * @brief UAVCAN Transport structure
 */
struct UavcanTransport {
    Socket* socket;
    NetInterface* interface;
    uint16_t local_port;
    bool multicast_enabled;
    OsMutex socket_mutex;
    bool initialized;
};

/**
 * @brief UDP endpoint structure for UAVCAN
 */
typedef struct {
    uint32_t ip_address;
    uint16_t udp_port;
} UavcanUdpEndpoint;

/**
 * @brief UAVCAN transport statistics
 */
typedef struct {
    bool initialized;
    bool multicast_enabled;
    bool socket_active;
    uint16_t local_port;
    uint32_t packets_sent;
    uint32_t packets_received;
    uint32_t send_errors;
    uint32_t receive_errors;
} UavcanTransportStats;

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Initialize UAVCAN transport
 * @param transport Pointer to transport structure
 * @param interface Pointer to network interface
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportInit(UavcanTransport* transport, NetInterface* interface);

/**
 * @brief Deinitialize UAVCAN transport
 * @param transport Pointer to transport structure
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportDeinit(UavcanTransport* transport);

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
                               size_t size);

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
                                  systime_t timeout);

/**
 * @brief Join multicast group
 * @param transport Pointer to transport structure
 * @param multicast_addr Multicast address to join
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportJoinMulticast(UavcanTransport* transport, 
                                        uint32_t multicast_addr);

/**
 * @brief Leave multicast group
 * @param transport Pointer to transport structure
 * @param multicast_addr Multicast address to leave
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportLeaveMulticast(UavcanTransport* transport, 
                                         uint32_t multicast_addr);

/**
 * @brief Create subject UDP endpoint
 * @param subject_id Subject ID
 * @param endpoint Pointer to endpoint structure to fill
 */
void uavcanTransportMakeSubjectEndpoint(UdpardPortID subject_id, 
                                       UavcanUdpEndpoint* endpoint);

/**
 * @brief Create service UDP endpoint
 * @param destination_node_id Destination node ID
 * @param endpoint Pointer to endpoint structure to fill
 */
void uavcanTransportMakeServiceEndpoint(UdpardNodeID destination_node_id, 
                                       UavcanUdpEndpoint* endpoint);

/**
 * @brief Check if transport is initialized
 * @param transport Pointer to transport structure
 * @retval bool True if initialized, false otherwise
 */
bool uavcanTransportIsInitialized(const UavcanTransport* transport);

/**
 * @brief Join UAVCAN subject multicast group
 * @param transport Pointer to transport structure
 * @param subject_id Subject ID to subscribe to
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportJoinSubjectMulticast(UavcanTransport* transport, 
                                               UdpardPortID subject_id);

/**
 * @brief Leave UAVCAN subject multicast group
 * @param transport Pointer to transport structure
 * @param subject_id Subject ID to unsubscribe from
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportLeaveSubjectMulticast(UavcanTransport* transport, 
                                                UdpardPortID subject_id);

/**
 * @brief Join UAVCAN service multicast group
 * @param transport Pointer to transport structure
 * @param node_id Node ID for service communication
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportJoinServiceMulticast(UavcanTransport* transport, 
                                               UdpardNodeID node_id);

/**
 * @brief Leave UAVCAN service multicast group
 * @param transport Pointer to transport structure
 * @param node_id Node ID for service communication
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportLeaveServiceMulticast(UavcanTransport* transport, 
                                                UdpardNodeID node_id);

/**
 * @brief Calculate UAVCAN subject multicast address
 * @param subject_id Subject ID
 * @retval uint32_t Multicast address in host byte order
 */
uint32_t uavcanTransportCalculateSubjectMulticast(UdpardPortID subject_id);

/**
 * @brief Calculate UAVCAN service multicast address
 * @param node_id Node ID
 * @retval uint32_t Multicast address in host byte order
 */
uint32_t uavcanTransportCalculateServiceMulticast(UdpardNodeID node_id);

/**
 * @brief Validate multicast address for UAVCAN
 * @param multicast_addr Multicast address to validate
 * @retval bool True if valid UAVCAN multicast address
 */
bool uavcanTransportIsValidMulticastAddr(uint32_t multicast_addr);

/**
 * @brief Get transport statistics
 * @param transport Pointer to transport structure
 * @param stats Pointer to statistics structure to fill
 * @retval UavcanError Error code
 */
UavcanError uavcanTransportGetStats(const UavcanTransport* transport, 
                                   UavcanTransportStats* stats);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_TRANSPORT_H */
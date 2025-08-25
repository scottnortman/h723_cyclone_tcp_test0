/**
 * @file uavcan_messages.h
 * @brief UAVCAN Message Handling interface
 * 
 * This file defines the interface for UAVCAN message handling including
 * message send/receive, serialization/deserialization, and subscription management.
 */

#ifndef UAVCAN_MESSAGES_H
#define UAVCAN_MESSAGES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "uavcan_types.h"
#include "uavcan_node.h"
#include "udpard.h"

/* Exported constants --------------------------------------------------------*/
#define UAVCAN_MESSAGE_MAX_SUBSCRIPTIONS        16
#define UAVCAN_MESSAGE_RX_QUEUE_SIZE           32
#define UAVCAN_MESSAGE_DEFAULT_TIMEOUT_MS      1000

/* Priority levels according to UAVCAN/Cyphal UDP standards */
#define UAVCAN_PRIORITY_EXCEPTIONAL           0  /* Highest priority */
#define UAVCAN_PRIORITY_IMMEDIATE             1
#define UAVCAN_PRIORITY_FAST                  2
#define UAVCAN_PRIORITY_HIGH                  3
#define UAVCAN_PRIORITY_NOMINAL               4  /* Default priority */
#define UAVCAN_PRIORITY_LOW                   5
#define UAVCAN_PRIORITY_SLOW                  6
#define UAVCAN_PRIORITY_OPTIONAL              7  /* Lowest priority */

/* Exported types ------------------------------------------------------------*/

/**
 * @brief UAVCAN message structure
 */
typedef struct {
    struct {
        UdpardPriority priority;
        UdpardNodeID source_node_id;
        UdpardNodeID destination_node_id;
        UdpardPortID subject_id;
        UdpardTransferID transfer_id;
    } header;
    
    struct {
        size_t size;
        void* data;
    } payload;
    
    struct {
        systime_t timestamp;
        uint32_t crc;
    } metadata;
} UavcanMessage;

/**
 * @brief UAVCAN subscription structure
 */
typedef struct {
    UdpardPortID subject_id;
    size_t extent;
    bool active;
    uint32_t messages_received;
    systime_t last_message_time;
} UavcanSubscription;

/**
 * @brief UAVCAN message handler structure
 */
typedef struct {
    /* Associated node */
    UavcanNode* node;
    
    /* RX port for subscriptions */
    struct UdpardRxPort rx_port;
    
    /* Memory resources for RX */
    struct UdpardRxMemoryResources rx_memory;
    
    /* Subscriptions management */
    UavcanSubscription subscriptions[UAVCAN_MESSAGE_MAX_SUBSCRIPTIONS];
    size_t subscription_count;
    
    /* Message queues */
    UavcanMessage rx_queue[UAVCAN_MESSAGE_RX_QUEUE_SIZE];
    size_t rx_queue_head;
    size_t rx_queue_tail;
    size_t rx_queue_count;
    
    /* Statistics */
    uint32_t messages_sent;
    uint32_t messages_received;
    uint32_t send_errors;
    uint32_t receive_errors;
    uint32_t subscription_errors;
    
    /* Synchronization */
    OsMutex handler_mutex;
    
    /* State */
    bool initialized;
} UavcanMessageHandler;

/**
 * @brief Message callback function type
 * @param message Pointer to received message
 * @param user_data User data passed to callback
 */
typedef void (*UavcanMessageCallback)(const UavcanMessage* message, void* user_data);

/* Exported function prototypes ----------------------------------------------*/

/**
 * @brief Initialize message handler
 * @param handler Pointer to message handler structure
 * @param node Pointer to associated UAVCAN node
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageHandlerInit(UavcanMessageHandler* handler, UavcanNode* node);

/**
 * @brief Deinitialize message handler
 * @param handler Pointer to message handler structure
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageHandlerDeinit(UavcanMessageHandler* handler);

/**
 * @brief Send UAVCAN message
 * @param handler Pointer to message handler structure
 * @param message Pointer to message to send
 * @param deadline_usec Transmission deadline in microseconds
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageSend(UavcanMessageHandler* handler, 
                             const UavcanMessage* message,
                             UdpardMicrosecond deadline_usec);

/**
 * @brief Receive UAVCAN message (non-blocking)
 * @param handler Pointer to message handler structure
 * @param message Pointer to message structure to fill
 * @param timeout_ms Timeout in milliseconds
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageReceive(UavcanMessageHandler* handler,
                                UavcanMessage* message,
                                systime_t timeout_ms);

/**
 * @brief Subscribe to a subject
 * @param handler Pointer to message handler structure
 * @param subject_id Subject ID to subscribe to
 * @param extent Maximum payload size for this subscription
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageSubscribe(UavcanMessageHandler* handler,
                                  UdpardPortID subject_id,
                                  size_t extent);

/**
 * @brief Unsubscribe from a subject
 * @param handler Pointer to message handler structure
 * @param subject_id Subject ID to unsubscribe from
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageUnsubscribe(UavcanMessageHandler* handler,
                                    UdpardPortID subject_id);

/**
 * @brief Process incoming UDP datagram
 * @param handler Pointer to message handler structure
 * @param datagram Pointer to received datagram
 * @param datagram_size Size of datagram
 * @param source_endpoint Source endpoint information
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageProcessDatagram(UavcanMessageHandler* handler,
                                        const void* datagram,
                                        size_t datagram_size,
                                        const UavcanUdpEndpoint* source_endpoint);

/**
 * @brief Create message with specified parameters
 * @param message Pointer to message structure to initialize
 * @param subject_id Subject ID
 * @param priority Message priority
 * @param transfer_id Transfer ID
 * @param payload_data Pointer to payload data
 * @param payload_size Size of payload data
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageCreate(UavcanMessage* message,
                               UdpardPortID subject_id,
                               UdpardPriority priority,
                               UdpardTransferID transfer_id,
                               const void* payload_data,
                               size_t payload_size);

/**
 * @brief Serialize message payload
 * @param message Pointer to message
 * @param buffer Pointer to output buffer
 * @param buffer_size Size of output buffer
 * @param serialized_size Pointer to store actual serialized size
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageSerialize(const UavcanMessage* message,
                                  void* buffer,
                                  size_t buffer_size,
                                  size_t* serialized_size);

/**
 * @brief Deserialize message payload
 * @param buffer Pointer to input buffer
 * @param buffer_size Size of input buffer
 * @param message Pointer to message structure to fill
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageDeserialize(const void* buffer,
                                    size_t buffer_size,
                                    UavcanMessage* message);

/**
 * @brief Get message priority name
 * @param priority Priority value
 * @retval const char* Priority name string
 */
const char* uavcanMessageGetPriorityName(UdpardPriority priority);

/**
 * @brief Validate message priority
 * @param priority Priority value to validate
 * @retval bool True if valid, false otherwise
 */
bool uavcanMessageIsValidPriority(UdpardPriority priority);

/**
 * @brief Get subscription by subject ID
 * @param handler Pointer to message handler structure
 * @param subject_id Subject ID to find
 * @retval UavcanSubscription* Pointer to subscription or NULL if not found
 */
UavcanSubscription* uavcanMessageFindSubscription(UavcanMessageHandler* handler,
                                                 UdpardPortID subject_id);

/**
 * @brief Get message handler statistics
 * @param handler Pointer to message handler structure
 * @param messages_sent Pointer to store sent message count
 * @param messages_received Pointer to store received message count
 * @param send_errors Pointer to store send error count
 * @param receive_errors Pointer to store receive error count
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageGetStatistics(const UavcanMessageHandler* handler,
                                      uint32_t* messages_sent,
                                      uint32_t* messages_received,
                                      uint32_t* send_errors,
                                      uint32_t* receive_errors);

/**
 * @brief Reset message handler statistics
 * @param handler Pointer to message handler structure
 * @retval UavcanError Error code
 */
UavcanError uavcanMessageResetStatistics(UavcanMessageHandler* handler);

/**
 * @brief Check if message handler is initialized
 * @param handler Pointer to message handler structure
 * @retval bool True if initialized, false otherwise
 */
bool uavcanMessageHandlerIsInitialized(const UavcanMessageHandler* handler);

#ifdef __cplusplus
}
#endif

#endif /* UAVCAN_MESSAGES_H */
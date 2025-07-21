
// TelnetTask.c
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"

// CycloneTCP core includes
#include "core/net.h"
#include "core/socket.h"
#include "core/tcp.h"
#include "ipv4/ipv4.h"
#include "error.h"

#include "TelnetTask.h"
#include <string.h>
#include <stdint.h>

#include "SerialTask.h" //debug

#define TELNET_PORT              23
#define TELNET_TASK_STACK_SIZE   512
#define CLI_BUFFER_SIZE          128

// Telnet IAC command and option codes
#define TELNET_IAC               255u
#define TELNET_WILL              251u
#define TELNET_WONT              252u
#define TELNET_ECHO              1u
#define TELNET_SUPPRESS_GO_AHEAD 3u

// Static stream buffer handles
static StreamBufferHandle_t xRxStream = NULL;
static StreamBufferHandle_t xTxStream = NULL;

// Forward declaration of the task function
static void prvTelnetTask(void *pvParameters);

StreamBufferHandle_t xTelnetTaskGetRxStreamHandle(void)
{
    return xRxStream;
}

StreamBufferHandle_t xTelnetTaskGetTxStreamHandle(void)
{
    return xTxStream;
}

BaseType_t xTelnetTaskStart(UBaseType_t uxPriority)
{
    // Create stream buffers if not already created
    if (xRxStream == NULL) {
        xRxStream = xStreamBufferCreate(256, 1);
    }
    if (xTxStream == NULL) {
        xTxStream = xStreamBufferCreate(256, 1);
    }
    if (xRxStream == NULL || xTxStream == NULL) {
        return pdFAIL;
    }

    // Start the Telnet listener task
    return xTaskCreate(prvTelnetTask,
                       "TelnetCLI",
                       TELNET_TASK_STACK_SIZE,
                       NULL,
                       uxPriority,
                       NULL);
}

static void prvTelnetTask(void *pvParameters)
{
    error_t err;
    size_t received;
    size_t written;

    // Open a TCP listening socket
    Socket *listener = socketOpen(SOCKET_TYPE_STREAM, IP_PROTOCOL_TCP);
    if (!listener) {
        vTaskDelete(NULL);
        return;
    }

    socketBind(listener, &IP_ADDR_ANY, TELNET_PORT);
    socketListen(listener, 1);

    for (;;) {

        // Wait for a client to connect
        Socket *client = socketAccept(listener, NULL, 0);
        if (!client) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

//        // 1) Negotiate Telnet options: server WILL ECHO, WILL SUPPRESS-GO-AHEAD
//        {
//            const uint8_t serverOpts[] = {
//                TELNET_IAC, TELNET_WILL, TELNET_ECHO,
//                TELNET_IAC, TELNET_WILL, TELNET_SUPPRESS_GO_AHEAD
//            };
//            //socketSend(client, serverOpts, sizeof(serverOpts), &written, 0);
//        }

        // 3) Prepare relay buffers, zero-initialized
        uint8_t inBuf[CLI_BUFFER_SIZE] = {0};
        uint8_t outBuf[CLI_BUFFER_SIZE] = {0};


        //Flush the socket; input buffer should be empty as we just displayed the CLI prompt
        socketReceive(client, inBuf, sizeof(inBuf), &received, 0);

        /**
         * Send a CR to have CLI display the prompt
         */
        const uint8_t CR = 0x0D;
        xStreamBufferSend( xRxStream, &CR, 1, portMAX_DELAY );

        // 4) Relay loop: shuttle bytes between socket and stream buffers
        /**
         * Recall the dataflow:
         * 	Telnet => xRxStream => CLI => xTxStream
         */
        for (;;) {

            // a) Receive data from client
            err = socketReceive(client, inBuf, sizeof(inBuf), &received, 0);
            if (err != NO_ERROR || received == 0) {
                break; // client closed or error
            }

            /**
             * DEBUG to send bytes received via telnet out the serial port
             */
//            for(uint16_t j=0; j<received; j++){
//            	vSerialPutChar(inBuf[j]);
//            }

            /**
             * This call is the interface to the CLI.
             */
            xStreamBufferSend(xRxStream, inBuf, received, portMAX_DELAY);

            vTaskDelay(10);

            // b) Drain console output (which includes console-driven echo) back to socket
            size_t avail = xStreamBufferBytesAvailable(xTxStream);

            while (avail > 0) {

                size_t chunk = (avail > sizeof(outBuf)) ? sizeof(outBuf) : avail;

                size_t n = xStreamBufferReceive(xTxStream, outBuf, chunk, 0);

                if (n > 0) {
                    err = socketSend(client, outBuf, n, &written, 0);
                    if (err != NO_ERROR) {
                        break;
                    }
                }
                avail = xStreamBufferBytesAvailable(xTxStream);
            }
        }

        // Clean up client socket
        socketClose(client);
    }
}

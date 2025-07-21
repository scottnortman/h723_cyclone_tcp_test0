/* CommandConsoleTask.h */
#ifndef INC_COMMANDCONSOLETASK_H_
#define INC_COMMANDCONSOLETASK_H_

#include "FreeRTOS.h"
#include "task.h"                         /* For task creation definitions */
#include "stream_buffer.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Default task config */
#ifndef CONSOLE_TASK_STACK_SIZE
#define CONSOLE_TASK_STACK_SIZE    256
#endif
#ifndef CONSOLE_TASK_PRIORITY
#define CONSOLE_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1 )
#endif

/* Buffer lengths */
#define CMD_INPUT_BUFFER_LENGTH   configCOMMAND_INT_MAX_INPUT_SIZE
#define CMD_OUTPUT_BUFFER_LENGTH  configCOMMAND_INT_MAX_OUTPUT_SIZE

/* Uncomment to enable raw character echo for IO testing */
 #define CONSOLE_ECHO_ENABLE

/**
 * @brief Initialize and start the Command Console task.
 * @param xRxStream    StreamBufferHandle_t for incoming bytes
 * @param xTxStream    StreamBufferHandle_t for outgoing bytes
 * @param uxPriority   Task priority (0 for default)
 * @param uxStackSize  Stack size (0 for default)
 */
void vCommandConsoleInit( StreamBufferHandle_t xRxStream,
                          StreamBufferHandle_t xTxStream,
                          UBaseType_t uxPriority,
                          UBaseType_t uxStackSize );

#ifdef __cplusplus
}
#endif
#endif /* INC_COMMANDCONSOLETASK_H_ */

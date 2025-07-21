/* SerialTask.h */
#ifndef INC_SERIALTASK_H_
#define INC_SERIALTASK_H_

#include <stdio.h>
#include "stm32h7xx_hal.h"
#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "semphr.h"

/* Sizes for RX and TX byte streams */
#define SERIAL_TASK_RX_BUFFER_SIZE   configCOMMAND_INT_MAX_INPUT_SIZE
#define SERIAL_TASK_TX_BUFFER_SIZE   configCOMMAND_INT_MAX_OUTPUT_SIZE
#define SERIAL_TASK_TRIGGER_LEVEL     1

/* If defined, a loopback test task (RX->TX echo) will be available. */
//#define SERIAL_TASK_LOOPBACK    /* comment out to disable loopback */

/* Initialize both RX and TX tasks (DMA, interrupts, buffers, mutex) */
void vSerialTaskInit(UBaseType_t xTxPriority, UBaseType_t xRxPriority);

#ifdef SERIAL_TASK_LOOPBACK
/* Start a loopback test task (RX->TX echo) */
void vSerialLoopbackTestStart(UBaseType_t uxPriority);
#endif

/* Get handles to the byte stream buffers */
StreamBufferHandle_t xSerialTaskGetRxStreamHandle(void);
StreamBufferHandle_t xSerialTaskGetTxStreamHandle(void);

/* Send helpers for byte stream */
void vSerialPutChar(char c);
void vSerialPutString(const char * buf, size_t len);

#endif /* INC_SERIALTASK_H_ */

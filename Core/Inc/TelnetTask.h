// TelnetTask.h
#ifndef TELNET_TASK_H
#define TELNET_TASK_H

#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "task.h"

/**
 * @brief  Get the handle to the Rx stream buffer for Telnet input.
 */
StreamBufferHandle_t xTelnetTaskGetRxStreamHandle(void);

/**
 * @brief  Get the handle to the Tx stream buffer for Telnet output.
 */
StreamBufferHandle_t xTelnetTaskGetTxStreamHandle(void);

/**
 * @brief  Create and start the TelnetTask.
 * @param  uxPriority  Task priority for the TelnetTask.
 * @return pdPASS on success, pdFAIL otherwise.
 */
BaseType_t xTelnetTaskStart(UBaseType_t uxPriority);

#endif // TELNET_TASK_H

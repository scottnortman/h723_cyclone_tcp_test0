/*
 * CommandConsoleDual.h
 *
 *  Created on: Jul 28, 2025
 *      Author: snortman
 */

#ifndef INC_COMMANDCONSOLEDUALTASK_H_
#define INC_COMMANDCONSOLEDUALTASK_H_

#include <stddef.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "stream_buffer.h"
#include "semphr.h"
#include "FreeRTOS_CLI.h"

/* Default task config */
#define COMMAND_CONSOLE_DUAL_TASK_STACK_SIZE	256
#define COMMAND_CONSOLE_DUAL_TASK_PRIORITY      ( tskIDLE_PRIORITY + 1 )

/* Uncomment to enable raw character echo for IO testing */
#define COMMAND_CONSOLE_DUAL_ECHO_ENABLE

#define COMMAND_CONSOLE_DUAL_WAIT_TIME	pdMS_TO_TICKS( 100 ) // ms

#define COMMAND_CONSOLE_DUAL_TASK_PRIO 	( tskIDLE_PRIORITY + 1 )



/**
 * @brief Initialize and start Serial and Telnet Command Console Tasks
 * @param xSerialRxStream, stream buffer handle to receive stream from serial
 * @param xSerialTxStream, stream buffer handle to transmit stream to serial
 * @param xTelnetRxStream, stream buffer handle to receive stream from telnet
 * @param xTelnetTxStream, stream buffer handle to transmit stream to telnet
 */
void vCommandConsoleDualInit(	StreamBufferHandle_t xSerialRxStream,
								StreamBufferHandle_t xSerialTxStream,
								StreamBufferHandle_t xTelnetRxStream,
								StreamBufferHandle_t xTelnetTxStream );


#endif /* INC_COMMANDCONSOLEDUALTASK_H_ */

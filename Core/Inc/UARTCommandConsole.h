/*
 * UARTCommandConsole.h
 *
 *  Created on: Jun 15, 2025
 *      Author: snortman
 */

#ifndef INC_UARTCOMMANDCONSOLE_H_
#define INC_UARTCOMMANDCONSOLE_H_

/* Standard includes. */
#include "string.h"
#include "stdio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* Example includes. */
#include "FreeRTOS_CLI.h"

/* Demo application includes. */
#include "serial.h"

/* Dimensions the buffer into which input characters are placed. */
#define cmdMAX_INPUT_SIZE          50

/* Dimensions a buffer to be used by the UART driver, if the UART driver uses a
 * buffer at all. */
#define cmdQUEUE_LENGTH            25

/* DEL acts as a backspace. */
#define cmdASCII_DEL               ( 0x7F )

/* The maximum time to wait for the mutex that guards the UART to become
 * available. */
#define cmdMAX_MUTEX_WAIT          pdMS_TO_TICKS( 300 )

#ifndef configCLI_BAUD_RATE
    #define configCLI_BAUD_RATE    115200
#endif

#ifdef __cpluplus
extern "C" {
#endif

BaseType_t xUARTCommandConsoleStart( uint16_t usStackSize, UBaseType_t uxPriority );

void xUARTCommandConsoleSetPort( xComPortHandle port );

#ifdef __cpluplus
}
#endif


#endif /* INC_UARTCOMMANDCONSOLE_H_ */

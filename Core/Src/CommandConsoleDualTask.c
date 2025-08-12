/*
 * CommandConsoleDualTask.c
 *
 *  Created on: Jul 28, 2025
 *      Author: snortman
 *
 *
 *
 *  This task implements a command console which supports dual serial and telnet
 *  interfaces simultaneously.
 */


#include "CommandConsoleDualTask.h"

/**
 *  Serial Interface Rx / Tx Buffers
 */
static StreamBufferHandle_t xSerialRxStreamBufferHandle = NULL; // Serial Rx Data console input
static StreamBufferHandle_t xSerialTxStreamBufferHandle = NULL;	// Serial Tx Data console output

/**
 * Telnet Interface Rx / Tx Buffers
 */
static StreamBufferHandle_t xTelnetRxStreamBufferHandle = NULL;	// Telnet Rx Data console input
static StreamBufferHandle_t xTelnetTxStreamBufferHandle = NULL; // Telnet Tx Data console output

/**
 * Mutex for shared CLI access
 */
static SemaphoreHandle_t xConsoleMutex = NULL;

static void prvCommandConsoleSerialTask( void * pvParams )
{

	char pcInput[configCOMMAND_INT_MAX_INPUT_SIZE];
	char pcOutput[configCOMMAND_INT_MAX_OUTPUT_SIZE];
	size_t uxIndex = 0;
	BaseType_t xMore;
	char c;

	(void) pvParams;

	while( 1 )
	{
		/**
		 * Wait for received characters via the serial port
		 */
		if( xStreamBufferReceive( xSerialRxStreamBufferHandle, &c, 1, portMAX_DELAY) > 0 )
		{
			#ifdef COMMAND_CONSOLE_DUAL_ECHO_ENABLE
			/* Echo back raw character if needed; not special chars */
			if( c != '>' && c != '\r' && c != '\n' )
			{
				xStreamBufferSend( xSerialTxStreamBufferHandle, &c, 1, portMAX_DELAY );
			}
			#endif

			/* Check for end of command */
			 if( ( c == '\r' ) || ( c == '\n' ) )
			 {
				 if( uxIndex > 0 )
				 {
					 // Null terminate input line
					 pcInput[uxIndex] = '\0';

					 /**
					  * Try to take mutex to access CLI; note if this fails, it may be due to a long-running
					  * command being executed on an alternative port, for example if a stream command is being
					  * executed. TODO:  figure out how to block long running commands
					  */
					 if( xSemaphoreTake( xConsoleMutex, COMMAND_CONSOLE_DUAL_WAIT_TIME ) == pdTRUE )
					 {
						 /* Process and send all CLI output */
						 do
						 {
							 xMore = FreeRTOS_CLIProcessCommand( pcInput,  pcOutput, configCOMMAND_INT_MAX_OUTPUT_SIZE );

							 if( strlen(pcOutput) > 0 )
							 {
								xStreamBufferSend( xSerialTxStreamBufferHandle, pcOutput, strlen( pcOutput ), portMAX_DELAY );
							 }

						 } while( xMore != pdFALSE );

						 // Reset for next line
						 uxIndex = 0;

						 /* Release mutex */
						 configASSERT( xSemaphoreGive( xConsoleMutex ) ); //TODO:  handle this better....

					 }
					 else
					 {
						 /* Unable to get mutex, return prompt only*/
						 char str[] = "\n\r>";
						 xStreamBufferSend( xSerialTxStreamBufferHandle, str, strlen(str), portMAX_DELAY );
					 }
				 }
				 else
				 {
					 // Received EOC without command, so return prompt only
					if( c == '\r')
					{
						//Send '>'
						char str[] = "\n\r>";
						xStreamBufferSend( xSerialTxStreamBufferHandle, str, strlen(str), portMAX_DELAY );
					}

				 }// else

			 }// if end of command
			 else
			 {
				/* Store character if space remains, else reset buffer */
				if( uxIndex < ( configCOMMAND_INT_MAX_INPUT_SIZE - 1 ) )
				{
					pcInput[ uxIndex++ ] = c;
				}
				else
				{
					uxIndex = 0;
				}

			 }//else

		}//end if char received

	}//end while(1)

} // prvCommandConsoleSerialTask


static void prvCommandConsoleTelnetTask( void * pvParams )
{
	char pcInput[configCOMMAND_INT_MAX_INPUT_SIZE];
	char pcOutput[configCOMMAND_INT_MAX_OUTPUT_SIZE];
	size_t uxIndex = 0;
	BaseType_t xMore;
	char c;

	(void) pvParams;

	while( 1 )
	{

		/**
		 * Wait for received characters via the serial port
		 */
		if( xStreamBufferReceive( xTelnetRxStreamBufferHandle, &c, 1, portMAX_DELAY) > 0 )
		{
			#ifdef COMMAND_CONSOLE_DUAL_ECHO_ENABLE
			/* Echo back raw character if needed; not special chars */
			if( c != '>' && c != '\r' && c != '\n' )
			{
				xStreamBufferSend( xTelnetTxStreamBufferHandle, &c, 1, portMAX_DELAY );
			}
			#endif

			/* Check for end of command */
			 if( ( c == '\r' ) || ( c == '\n' ) )
			 {
				 if( uxIndex > 0 )
				 {
					 // Null terminate input line
					 pcInput[uxIndex] = '\0';

					 /**
					  * Try to take mutex to access CLI; note if this fails, it may be due to a long-running
					  * command being executed on an alternative port, for example if a stream command is being
					  * executed. TODO:  figure out how to block long running commands
					  */
					 if( xSemaphoreTake( xConsoleMutex, COMMAND_CONSOLE_DUAL_WAIT_TIME ) == pdTRUE )
					 {
						 /* Process and send all CLI output */
						 do
						 {
							 xMore = FreeRTOS_CLIProcessCommand( pcInput,  pcOutput, configCOMMAND_INT_MAX_OUTPUT_SIZE );

							 if( strlen(pcOutput) > 0 )
							 {
								xStreamBufferSend( xTelnetTxStreamBufferHandle, pcOutput, strlen( pcOutput ), portMAX_DELAY );
							 }

						 } while( xMore != pdFALSE );

						 // Reset for next line
						 uxIndex = 0;

						 /* Release mutex */
						 configASSERT( xSemaphoreGive( xConsoleMutex ) ); //TODO:  handle this better....

					 }
					 else
					 {
						 /* Unable to get mutex, return prompt only*/
						 char str[] = "\r>";
						 xStreamBufferSend( xTelnetTxStreamBufferHandle, str, strlen(str), portMAX_DELAY );
					 }
				 }
				 else
				 {
					 // Received EOC without command, so return prompt only
					if( c == '\r' )
					{
						//Send '>'
						char str[] = "\r>"; //no newline for telnet
						xStreamBufferSend( xTelnetTxStreamBufferHandle, str, strlen(str), portMAX_DELAY );
					}

				 }

			 }// if end of command
			 else
			 {
				/* Store character if space remains, else reset buffer */
				if( uxIndex < ( configCOMMAND_INT_MAX_INPUT_SIZE - 1 ) )
				{
					pcInput[ uxIndex++ ] = c;
				}
				else
				{
					uxIndex = 0;
				}

			 }

		}//end if char received

	}//end while(1)

} // prvCommandConsoleTelnetTask

void vCommandConsoleDualInit(	StreamBufferHandle_t xSerialRxStream,
								StreamBufferHandle_t xSerialTxStream,
								StreamBufferHandle_t xTelnetRxStream,
								StreamBufferHandle_t xTelnetTxStream )
{

	BaseType_t ret;

	/**
	 * Confirm buffers are valid
	 */
	xSerialRxStreamBufferHandle = xSerialRxStream;
	configASSERT( xSerialRxStreamBufferHandle );

	xSerialTxStreamBufferHandle = xSerialTxStream;
	configASSERT( xSerialTxStreamBufferHandle );

	xTelnetRxStreamBufferHandle = xTelnetRxStream;
	configASSERT( xTelnetRxStreamBufferHandle );

	xTelnetTxStreamBufferHandle = xTelnetTxStream;
	configASSERT( xTelnetTxStreamBufferHandle );

	/**
	 * Initialize mutex
	 */
	xConsoleMutex = xSemaphoreCreateMutex();
	configASSERT( xConsoleMutex );

	/**
	 * Start tasks...
	 */
	ret = xTaskCreate( prvCommandConsoleSerialTask, "CmdDualSerial", COMMAND_CONSOLE_DUAL_TASK_STACK_SIZE, NULL, COMMAND_CONSOLE_DUAL_TASK_PRIO, NULL );
	configASSERT( ret == pdPASS );

	ret = xTaskCreate( prvCommandConsoleTelnetTask, "CmdDualTelnet", COMMAND_CONSOLE_DUAL_TASK_STACK_SIZE, NULL, COMMAND_CONSOLE_DUAL_TASK_PRIO, NULL );
	configASSERT( ret == pdPASS );

} // vCommandConsoleDualTask





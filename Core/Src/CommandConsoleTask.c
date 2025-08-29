/* CommandConsoleTask.c */
#include "CommandConsoleTask.h"
#include "FreeRTOS_CLI.h"
#include <string.h>

#include "SerialTask.h" //debug

static StreamBufferHandle_t xConsoleRxStream = NULL;
static StreamBufferHandle_t xConsoleTxStream = NULL;

static void prvCommandConsoleTask( void * pvParameters )
{
    char pcInput[ CMD_INPUT_BUFFER_LENGTH ];
    char pcOutput[ CMD_OUTPUT_BUFFER_LENGTH ];
    size_t uxIndex = 0;
    BaseType_t xMore;
    char c;

    /* Suppress unused parameter warning */
    ( void ) pvParameters;

    for( ;; )
    {
        /* Wait indefinitely for one character */
        if( xStreamBufferReceive( xConsoleRxStream, &c, 1, portMAX_DELAY ) > 0 )
        {

#ifdef CONSOLE_ECHO_ENABLE
            /* Echo back raw character for IO test (block until space available) */
        	if( c != '>' && c != '\r' && c != '\n' ){
        		xStreamBufferSend( xConsoleTxStream, &c, 1, portMAX_DELAY );
        	}
#endif
            /* End-of-line? Process command */
            if( ( c == '\r' ) || ( c == '\n' ) )
            {
                if( uxIndex > 0 )
                {
                    /* Null-terminate the input line */
                    pcInput[ uxIndex ] = '\0';

                    /* Process and send all CLI output */
                    do
                    {
                        /* Clear output buffer before processing */
                        memset(pcOutput, 0, CMD_OUTPUT_BUFFER_LENGTH);
                        
                        xMore = FreeRTOS_CLIProcessCommand( pcInput,  pcOutput, CMD_OUTPUT_BUFFER_LENGTH );

                        if( strlen(pcOutput) > 0 ){
                        	xStreamBufferSend( xConsoleTxStream, pcOutput, strlen( pcOutput ), portMAX_DELAY );
                        }

                    } while( xMore != pdFALSE );

                    /* Reset for next line */
                    uxIndex = 0;

                    /**
                     *
                     */
                    if( c =='\r')
                    {
						char str[] = "\r>";
						xStreamBufferSend( xConsoleTxStream, str, strlen(str), portMAX_DELAY );
                    }

                }
                else
                {
                	if( c =='\r')
                	{
						//Send '>'
						char str[] = "\n\r>";
						xStreamBufferSend( xConsoleTxStream, str, strlen(str), portMAX_DELAY );
                	}
                }
            }
            else
            {
                /* Store character if space remains, else reset buffer */
                if( uxIndex < ( CMD_INPUT_BUFFER_LENGTH - 1 ) )
                {
                    pcInput[ uxIndex++ ] = c;
                }
                else
                {
                    uxIndex = 0;
                }
            }
        }
    }
}

void vCommandConsoleInit( StreamBufferHandle_t xRxStream,
                          StreamBufferHandle_t xTxStream,
                          UBaseType_t uxPriority,
                          UBaseType_t uxStackSize )
{
    /* Save the I/O handles */
    xConsoleRxStream = xRxStream;
    xConsoleTxStream = xTxStream;

    /* Use default priority if none provided */
    if( uxPriority == 0 )
    {
        uxPriority = CONSOLE_TASK_PRIORITY;
    }

    /* Use default stack size if none provided */
    if( uxStackSize == 0 )
    {
        uxStackSize = CONSOLE_TASK_STACK_SIZE;
    }

    /* Create the console task */
    BaseType_t xResult = xTaskCreate( prvCommandConsoleTask,
                                       "CmdConsole",
                                       uxStackSize,
                                       NULL,
                                       uxPriority,
                                       NULL );
    configASSERT( xResult == pdPASS );
}

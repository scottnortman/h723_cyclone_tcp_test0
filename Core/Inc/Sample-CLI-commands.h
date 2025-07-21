/*
 * Sample-CLI-command.h
 *
 *  Created on: Jun 20, 2025
 *      Author: snortman
 */

#ifndef INC_SAMPLE_CLI_COMMANDS_H_
#define INC_SAMPLE_CLI_COMMANDS_H_


/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* FreeRTOS+CLI includes. */
#include "FreeRTOS_CLI.h"

#ifndef  configINCLUDE_TRACE_RELATED_CLI_COMMANDS
    #define configINCLUDE_TRACE_RELATED_CLI_COMMANDS    0
#endif

#ifndef configINCLUDE_QUERY_HEAP_COMMAND
    #define configINCLUDE_QUERY_HEAP_COMMAND    0
#endif



/*
 * The function that registers the commands that are defined within this file.
 */
void vRegisterSampleCLICommands( void );


#endif /* INC_SAMPLE_CLI_COMMANDS_H_ */

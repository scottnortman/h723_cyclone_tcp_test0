/**
 * @file freertos_hooks.c
 * @brief FreeRTOS Hook Functions
 * 
 * This file contains FreeRTOS hook functions for debugging and monitoring,
 * particularly for stack overflow detection and system health monitoring.
 */

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

/* Global variables for stack overflow tracking */
static volatile uint32_t g_stackOverflowCount = 0;
static volatile char g_lastOverflowTaskName[configMAX_TASK_NAME_LEN] = {0};
static volatile uint32_t g_lastOverflowTaskNumber = 0;

/**
 * @brief Stack overflow hook function
 * 
 * This function is called when FreeRTOS detects a stack overflow.
 * It's configured by setting configCHECK_FOR_STACK_OVERFLOW to 2.
 * 
 * @param xTask Handle of the task that overflowed
 * @param pcTaskName Name of the task that overflowed
 */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    /* Increment overflow counter */
    g_stackOverflowCount++;
    
    /* Store the task information */
    if (pcTaskName != NULL) {
        strncpy((char*)g_lastOverflowTaskName, pcTaskName, configMAX_TASK_NAME_LEN - 1);
        g_lastOverflowTaskName[configMAX_TASK_NAME_LEN - 1] = '\0';
    }
    
    if (xTask != NULL) {
        g_lastOverflowTaskNumber = uxTaskGetTaskNumber(xTask);
    }
    
    /* Flash LED to indicate error */
    BSP_LED_On(LED_RED);
    
    /* Disable interrupts and halt - this is a critical error */
    taskDISABLE_INTERRUPTS();
    
    /* Infinite loop - system must be reset */
    for (;;) {
        /* Optional: Toggle LED to show we're in error state */
        static uint32_t counter = 0;
        if (++counter > 1000000) {
            BSP_LED_Toggle(LED_RED);
            counter = 0;
        }
    }
}

/**
 * @brief Malloc failed hook function
 * 
 * This function is called when pvPortMalloc() fails to allocate memory.
 * It helps debug heap exhaustion issues.
 */
void vApplicationMallocFailedHook(void)
{
    /* Flash LED to indicate memory allocation failure */
    BSP_LED_On(LED_RED);
    
    /* Disable interrupts and halt */
    taskDISABLE_INTERRUPTS();
    
    /* Infinite loop - system must be reset */
    for (;;) {
        /* Optional: Different flash pattern for malloc failure */
        static uint32_t counter = 0;
        if (++counter > 500000) {
            BSP_LED_Toggle(LED_RED);
            counter = 0;
        }
    }
}

/**
 * @brief Idle hook function
 * 
 * This function is called during each cycle of the idle task.
 * Can be used for low-priority background tasks or power management.
 */
void vApplicationIdleHook(void)
{
    /* This hook function is called during each cycle of the idle task.
     * It can be used for low-priority background tasks.
     * 
     * NOTE: vApplicationIdleHook() MUST NOT, under any circumstances, 
     * call a function that might block (for example, vTaskDelay(), 
     * or a queue or semaphore function with a non-zero timeout).
     */
    
    /* Optional: Put CPU into low power mode */
    /* __WFI(); */
}

/* Note: vApplicationTickHook is already implemented in main.c */

/**
 * @brief Get stack overflow statistics
 * 
 * This function provides access to stack overflow tracking information
 * for debugging purposes.
 * 
 * @param pxOverflowCount Pointer to store overflow count
 * @param pcLastTaskName Buffer to store last overflow task name
 * @param xBufferSize Size of the task name buffer
 * @param pxLastTaskNumber Pointer to store last overflow task number
 */
void vGetStackOverflowInfo(uint32_t *pxOverflowCount, 
                          char *pcLastTaskName, 
                          size_t xBufferSize,
                          uint32_t *pxLastTaskNumber)
{
    if (pxOverflowCount != NULL) {
        *pxOverflowCount = g_stackOverflowCount;
    }
    
    if (pcLastTaskName != NULL && xBufferSize > 0) {
        strncpy(pcLastTaskName, (const char*)g_lastOverflowTaskName, xBufferSize - 1);
        pcLastTaskName[xBufferSize - 1] = '\0';
    }
    
    if (pxLastTaskNumber != NULL) {
        *pxLastTaskNumber = g_lastOverflowTaskNumber;
    }
}

/**
 * @brief Reset stack overflow statistics
 * 
 * This function resets the stack overflow tracking counters.
 * Useful for testing and debugging.
 */
void vResetStackOverflowInfo(void)
{
    g_stackOverflowCount = 0;
    memset((void*)g_lastOverflowTaskName, 0, sizeof(g_lastOverflowTaskName));
    g_lastOverflowTaskNumber = 0;
}
/**
 * @file stack_monitor_cli.c
 * @brief FreeRTOS Stack Monitoring CLI Commands
 * 
 * This module provides comprehensive CLI commands for monitoring FreeRTOS task
 * stack usage, detecting stack overflows, and debugging stack-related issues.
 * 
 * Commands provided:
 * - stack-info: Show stack usage for all tasks
 * - stack-check: Check for stack overflow conditions
 * - stack-watch: Monitor specific task stack usage
 * - heap-info: Show heap usage statistics
 * - memory-info: Show comprehensive memory information
 */

#include "stack_monitor_cli.h"
#include "freertos_hooks.h"
#include "FreeRTOS.h"
#include "task.h"
#include "FreeRTOS_CLI.h"
#include <stdio.h>
#include <string.h>

/* Forward declarations */
static BaseType_t prvStackInfoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvStackCheckCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvStackWatchCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvHeapInfoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvMemoryInfoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);
static BaseType_t prvStackOverflowInfoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

/* Helper functions */
static UBaseType_t prvGetTaskStackHighWaterMark(TaskHandle_t xTask);
static void prvFormatStackInfo(char *pcBuffer, size_t xBufferLen, const char *pcTaskName, 
                              UBaseType_t uxStackSize, UBaseType_t uxHighWaterMark);

/* CLI Command Definitions */
static const CLI_Command_Definition_t xStackInfoCommand = {
    "stack-info",
    "\r\nstack-info:\r\n Show stack usage information for all tasks\r\n",
    prvStackInfoCommand,
    0
};

static const CLI_Command_Definition_t xStackCheckCommand = {
    "stack-check",
    "\r\nstack-check:\r\n Check for stack overflow conditions and warnings\r\n",
    prvStackCheckCommand,
    0
};

static const CLI_Command_Definition_t xStackWatchCommand = {
    "stack-watch",
    "\r\nstack-watch [task-name]:\r\n Monitor specific task stack usage (or all tasks if no name given)\r\n",
    prvStackWatchCommand,
    -1  /* Variable number of parameters */
};

static const CLI_Command_Definition_t xHeapInfoCommand = {
    "heap-info",
    "\r\nheap-info:\r\n Show detailed heap usage statistics\r\n",
    prvHeapInfoCommand,
    0
};

static const CLI_Command_Definition_t xMemoryInfoCommand = {
    "memory-info",
    "\r\nmemory-info:\r\n Show comprehensive memory usage information\r\n",
    prvMemoryInfoCommand,
    0
};

static const CLI_Command_Definition_t xStackOverflowInfoCommand = {
    "stack-overflow-info",
    "\r\nstack-overflow-info:\r\n Show stack overflow detection history and statistics\r\n",
    prvStackOverflowInfoCommand,
    0
};

/**
 * @brief Register all stack monitoring CLI commands
 */
void vRegisterStackMonitorCLICommands(void)
{
    FreeRTOS_CLIRegisterCommand(&xStackInfoCommand);
    FreeRTOS_CLIRegisterCommand(&xStackCheckCommand);
    FreeRTOS_CLIRegisterCommand(&xStackWatchCommand);
    FreeRTOS_CLIRegisterCommand(&xHeapInfoCommand);
    FreeRTOS_CLIRegisterCommand(&xMemoryInfoCommand);
    FreeRTOS_CLIRegisterCommand(&xStackOverflowInfoCommand);
}

/**
 * @brief Show stack usage information for all tasks
 */
static BaseType_t prvStackInfoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    (void)pcCommandString;
    
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    UBaseType_t uxHighWaterMark;
    char *pcWriteBufferStart = pcWriteBuffer;
    size_t xRemainingBufferLen = xWriteBufferLen;
    int written;
    
    /* Take a snapshot of the number of tasks in case it changes while this
       function is executing. */
    uxArraySize = uxTaskGetNumberOfTasks();
    
    /* Allocate an array to hold the task status structures. */
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (pxTaskStatusArray != NULL) {
        /* Generate the (binary) data. */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
        
        /* Write header */
        written = snprintf(pcWriteBuffer, xRemainingBufferLen,
            "Stack Usage Report:\r\n"
            "Task Name        Stack Size  Used   Free   Usage%%  Status\r\n"
            "--------------------------------------------------------\r\n");
        
        if (written > 0 && written < (int)xRemainingBufferLen) {
            pcWriteBuffer += written;
            xRemainingBufferLen -= written;
        }
        
        /* Process each task */
        for (x = 0; x < uxArraySize; x++) {
            uxHighWaterMark = uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle);
            
            /* High water mark is the FREE stack space remaining */
            UBaseType_t uxFree = uxHighWaterMark;
            
            /* We need to get the total stack size for this task */
            /* For now, use a reasonable estimate based on common stack sizes */
            UBaseType_t uxTotal = 1024; /* Default assumption - will be improved */
            
            /* Try to determine actual stack size from task name patterns */
            if (strstr(pxTaskStatusArray[x].pcTaskName, "IDLE") != NULL) {
                uxTotal = configMINIMAL_STACK_SIZE * sizeof(StackType_t);
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Telnet") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Serial") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);  /* Updated from 512 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Cmd") != NULL) {
                uxTotal = 2048 * sizeof(StackType_t);  /* Updated from 1024 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "RED") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);  /* Updated from 512 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "GRN") != NULL) {
                uxTotal = 512 * sizeof(StackType_t);
            }
            
            UBaseType_t uxUsed = uxTotal - uxFree;
            UBaseType_t uxUsagePercent = (uxTotal > 0) ? (uxUsed * 100) / uxTotal : 0;
            
            const char *pcStatus = "OK";
            if (uxUsagePercent > 90) {
                pcStatus = "CRITICAL";
            } else if (uxUsagePercent > 75) {
                pcStatus = "WARNING";
            }
            
            written = snprintf(pcWriteBuffer, xRemainingBufferLen,
                "%-15s  %8lu  %5lu  %5lu   %3lu%%   %s\r\n",
                pxTaskStatusArray[x].pcTaskName,
                (unsigned long)uxTotal,
                (unsigned long)uxUsed,
                (unsigned long)uxFree,
                (unsigned long)uxUsagePercent,
                pcStatus);
            
            if (written > 0 && written < (int)xRemainingBufferLen) {
                pcWriteBuffer += written;
                xRemainingBufferLen -= written;
            } else {
                break; /* Buffer full */
            }
        }
        
        /* Add summary */
        written = snprintf(pcWriteBuffer, xRemainingBufferLen,
            "--------------------------------------------------------\r\n"
            "Total Tasks: %lu\r\n",
            (unsigned long)uxArraySize);
        
        vPortFree(pxTaskStatusArray);
    } else {
        snprintf(pcWriteBuffer, xWriteBufferLen,
            "Error: Unable to allocate memory for task status array\r\n");
    }
    
    return pdFALSE;
}

/**
 * @brief Check for stack overflow conditions and warnings
 */
static BaseType_t prvStackCheckCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    (void)pcCommandString;
    
    TaskStatus_t *pxTaskStatusArray;
    volatile UBaseType_t uxArraySize, x;
    UBaseType_t uxHighWaterMark;
    char *pcWriteBufferStart = pcWriteBuffer;
    size_t xRemainingBufferLen = xWriteBufferLen;
    int written;
    UBaseType_t uxCriticalTasks = 0, uxWarningTasks = 0;
    
    /* Take a snapshot of the number of tasks */
    uxArraySize = uxTaskGetNumberOfTasks();
    
    /* Allocate array for task status */
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (pxTaskStatusArray != NULL) {
        /* Get system state */
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
        
        /* Write header */
        written = snprintf(pcWriteBuffer, xRemainingBufferLen,
            "Stack Overflow Check Results:\r\n"
            "=============================\r\n");
        
        if (written > 0 && written < (int)xRemainingBufferLen) {
            pcWriteBuffer += written;
            xRemainingBufferLen -= written;
        }
        
        /* Check each task for potential issues */
        for (x = 0; x < uxArraySize; x++) {
            uxHighWaterMark = uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle);
            
            /* High water mark is the FREE stack space remaining */
            UBaseType_t uxFree = uxHighWaterMark;
            
            /* Estimate total stack size based on task name */
            UBaseType_t uxTotal = 1024;
            if (strstr(pxTaskStatusArray[x].pcTaskName, "IDLE") != NULL) {
                uxTotal = configMINIMAL_STACK_SIZE * sizeof(StackType_t);
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Telnet") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Serial") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);  /* Updated from 512 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Cmd") != NULL) {
                uxTotal = 2048 * sizeof(StackType_t);  /* Updated from 1024 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "RED") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);  /* Updated from 512 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "GRN") != NULL) {
                uxTotal = 512 * sizeof(StackType_t);
            }
            
            UBaseType_t uxUsed = uxTotal - uxFree;
            UBaseType_t uxUsagePercent = (uxUsed * 100) / uxTotal;
            
            if (uxUsagePercent > 90) {
                written = snprintf(pcWriteBuffer, xRemainingBufferLen,
                    "CRITICAL: %s - %lu%% stack usage (%lu/%lu bytes)\r\n",
                    pxTaskStatusArray[x].pcTaskName,
                    (unsigned long)uxUsagePercent,
                    (unsigned long)uxUsed,
                    (unsigned long)uxTotal);
                uxCriticalTasks++;
                
                if (written > 0 && written < (int)xRemainingBufferLen) {
                    pcWriteBuffer += written;
                    xRemainingBufferLen -= written;
                }
            } else if (uxUsagePercent > 75) {
                written = snprintf(pcWriteBuffer, xRemainingBufferLen,
                    "WARNING: %s - %lu%% stack usage (%lu/%lu bytes)\r\n",
                    pxTaskStatusArray[x].pcTaskName,
                    (unsigned long)uxUsagePercent,
                    (unsigned long)uxUsed,
                    (unsigned long)uxTotal);
                uxWarningTasks++;
                
                if (written > 0 && written < (int)xRemainingBufferLen) {
                    pcWriteBuffer += written;
                    xRemainingBufferLen -= written;
                }
            }
        }
        
        /* Summary */
        if (uxCriticalTasks == 0 && uxWarningTasks == 0) {
            written = snprintf(pcWriteBuffer, xRemainingBufferLen,
                "All tasks have healthy stack usage levels.\r\n");
        } else {
            written = snprintf(pcWriteBuffer, xRemainingBufferLen,
                "\r\nSummary: %lu critical, %lu warning tasks found.\r\n"
                "Recommendation: Increase stack size for critical tasks.\r\n",
                (unsigned long)uxCriticalTasks,
                (unsigned long)uxWarningTasks);
        }
        
        vPortFree(pxTaskStatusArray);
    } else {
        snprintf(pcWriteBuffer, xWriteBufferLen,
            "Error: Unable to allocate memory for stack check\r\n");
    }
    
    return pdFALSE;
}

/**
 * @brief Monitor specific task stack usage
 */
static BaseType_t prvStackWatchCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    const char *pcParameter;
    BaseType_t xParameterStringLength;
    TaskHandle_t xTaskHandle = NULL;
    UBaseType_t uxHighWaterMark;
    TaskStatus_t xTaskDetails;
    
    /* Get the parameter string (task name) */
    pcParameter = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    
    if (pcParameter != NULL) {
        /* Find the task by name */
        char taskName[configMAX_TASK_NAME_LEN + 1];
        memset(taskName, 0, sizeof(taskName));
        strncpy(taskName, pcParameter, xParameterStringLength);
        
        /* Get task handle by name (we need to iterate through all tasks) */
        TaskStatus_t *pxTaskStatusArray;
        UBaseType_t uxArraySize, x;
        
        uxArraySize = uxTaskGetNumberOfTasks();
        pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
        
        if (pxTaskStatusArray != NULL) {
            uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
            
            for (x = 0; x < uxArraySize; x++) {
                if (strncmp(pxTaskStatusArray[x].pcTaskName, taskName, strlen(taskName)) == 0) {
                    xTaskHandle = pxTaskStatusArray[x].xHandle;
                    xTaskDetails = pxTaskStatusArray[x];
                    break;
                }
            }
            vPortFree(pxTaskStatusArray);
        }
        
        if (xTaskHandle != NULL) {
            uxHighWaterMark = uxTaskGetStackHighWaterMark(xTaskHandle);
            
            /* High water mark is the FREE stack space remaining */
            UBaseType_t uxFree = uxHighWaterMark;
            
            /* Estimate total stack size */
            UBaseType_t uxTotal = 1024;
            if (strstr(xTaskDetails.pcTaskName, "IDLE") != NULL) {
                uxTotal = configMINIMAL_STACK_SIZE * sizeof(StackType_t);
            } else if (strstr(xTaskDetails.pcTaskName, "Telnet") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);
            } else if (strstr(xTaskDetails.pcTaskName, "Serial") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);  /* Updated from 512 */
            } else if (strstr(xTaskDetails.pcTaskName, "Cmd") != NULL) {
                uxTotal = 2048 * sizeof(StackType_t);  /* Updated from 1024 */
            } else if (strstr(xTaskDetails.pcTaskName, "RED") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);  /* Updated from 512 */
            } else if (strstr(xTaskDetails.pcTaskName, "GRN") != NULL) {
                uxTotal = 512 * sizeof(StackType_t);
            }
            
            UBaseType_t uxUsed = uxTotal - uxFree;
            UBaseType_t uxUsagePercent = (uxUsed * 100) / uxTotal;
            
            snprintf(pcWriteBuffer, xWriteBufferLen,
                "Stack Watch - Task: %s\r\n"
                "=======================\r\n"
                "Stack Size:     %lu bytes\r\n"
                "Used:           %lu bytes\r\n"
                "Free:           %lu bytes\r\n"
                "Usage:          %lu%%\r\n"
                "Priority:       %lu\r\n"
                "State:          %s\r\n"
                "Status:         %s\r\n",
                taskName,
                (unsigned long)uxTotal,
                (unsigned long)uxUsed,
                (unsigned long)uxFree,
                (unsigned long)uxUsagePercent,
                (unsigned long)xTaskDetails.uxCurrentPriority,
                (xTaskDetails.eCurrentState == eRunning) ? "Running" :
                (xTaskDetails.eCurrentState == eReady) ? "Ready" :
                (xTaskDetails.eCurrentState == eBlocked) ? "Blocked" :
                (xTaskDetails.eCurrentState == eSuspended) ? "Suspended" : "Unknown",
                (uxUsagePercent > 90) ? "CRITICAL" :
                (uxUsagePercent > 75) ? "WARNING" : "OK");
        } else {
            snprintf(pcWriteBuffer, xWriteBufferLen,
                "Error: Task '%s' not found\r\n", taskName);
        }
    } else {
        /* No parameter given, show all tasks briefly */
        snprintf(pcWriteBuffer, xWriteBufferLen,
            "Usage: stack-watch <task-name>\r\n"
            "Use 'task-stats' to see all task names\r\n");
    }
    
    return pdFALSE;
}

/**
 * @brief Show detailed heap usage statistics
 */
static BaseType_t prvHeapInfoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    (void)pcCommandString;
    
    size_t xFreeHeapSize = xPortGetFreeHeapSize();
    size_t xMinimumEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
    size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;
    size_t xUsedHeapSize = xTotalHeapSize - xFreeHeapSize;
    size_t xMaxUsedHeapSize = xTotalHeapSize - xMinimumEverFreeHeapSize;
    
    UBaseType_t uxCurrentUsagePercent = (xUsedHeapSize * 100) / xTotalHeapSize;
    UBaseType_t uxMaxUsagePercent = (xMaxUsedHeapSize * 100) / xTotalHeapSize;
    
    snprintf(pcWriteBuffer, xWriteBufferLen,
        "Heap Usage Statistics:\r\n"
        "=====================\r\n"
        "Total Heap Size:        %lu bytes\r\n"
        "Currently Used:         %lu bytes (%lu%%)\r\n"
        "Currently Free:         %lu bytes\r\n"
        "Maximum Ever Used:      %lu bytes (%lu%%)\r\n"
        "Minimum Ever Free:      %lu bytes\r\n"
        "Fragmentation Risk:     %s\r\n"
        "Status:                 %s\r\n",
        (unsigned long)xTotalHeapSize,
        (unsigned long)xUsedHeapSize,
        (unsigned long)uxCurrentUsagePercent,
        (unsigned long)xFreeHeapSize,
        (unsigned long)xMaxUsedHeapSize,
        (unsigned long)uxMaxUsagePercent,
        (unsigned long)xMinimumEverFreeHeapSize,
        (uxMaxUsagePercent > 85) ? "HIGH" : 
        (uxMaxUsagePercent > 70) ? "MEDIUM" : "LOW",
        (uxCurrentUsagePercent > 90) ? "CRITICAL" :
        (uxCurrentUsagePercent > 75) ? "WARNING" : "OK");
    
    return pdFALSE;
}

/**
 * @brief Show comprehensive memory usage information
 */
static BaseType_t prvMemoryInfoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    (void)pcCommandString;
    
    /* Heap information */
    size_t xFreeHeapSize = xPortGetFreeHeapSize();
    size_t xMinimumEverFreeHeapSize = xPortGetMinimumEverFreeHeapSize();
    size_t xTotalHeapSize = configTOTAL_HEAP_SIZE;
    size_t xUsedHeapSize = xTotalHeapSize - xFreeHeapSize;
    
    /* Task count */
    UBaseType_t uxTaskCount = uxTaskGetNumberOfTasks();
    
    /* Calculate approximate stack usage */
    TaskStatus_t *pxTaskStatusArray;
    UBaseType_t uxArraySize, x;
    UBaseType_t uxTotalStackAllocated = 0;
    UBaseType_t uxTotalStackUsed = 0;
    
    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    
    if (pxTaskStatusArray != NULL) {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
        
        for (x = 0; x < uxArraySize; x++) {
            UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(pxTaskStatusArray[x].xHandle);
            
            /* High water mark is the FREE stack space remaining */
            UBaseType_t uxFree = uxHighWaterMark;
            
            /* Estimate total stack size */
            UBaseType_t uxTotal = 1024;
            if (strstr(pxTaskStatusArray[x].pcTaskName, "IDLE") != NULL) {
                uxTotal = configMINIMAL_STACK_SIZE * sizeof(StackType_t);
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Telnet") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Serial") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);  /* Updated from 512 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "Cmd") != NULL) {
                uxTotal = 2048 * sizeof(StackType_t);  /* Updated from 1024 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "RED") != NULL) {
                uxTotal = 1024 * sizeof(StackType_t);  /* Updated from 512 */
            } else if (strstr(pxTaskStatusArray[x].pcTaskName, "GRN") != NULL) {
                uxTotal = 512 * sizeof(StackType_t);
            }
            
            UBaseType_t uxUsed = uxTotal - uxFree;
            
            uxTotalStackAllocated += uxTotal;
            uxTotalStackUsed += uxUsed;
        }
        vPortFree(pxTaskStatusArray);
    }
    
    snprintf(pcWriteBuffer, xWriteBufferLen,
        "Comprehensive Memory Report:\r\n"
        "============================\r\n"
        "HEAP MEMORY:\r\n"
        "  Total Size:           %lu bytes\r\n"
        "  Used:                 %lu bytes (%lu%%)\r\n"
        "  Free:                 %lu bytes\r\n"
        "  Min Ever Free:        %lu bytes\r\n"
        "\r\n"
        "STACK MEMORY:\r\n"
        "  Total Allocated:      %lu bytes\r\n"
        "  Total Used:           %lu bytes (%lu%%)\r\n"
        "  Total Free:           %lu bytes\r\n"
        "\r\n"
        "SYSTEM:\r\n"
        "  Active Tasks:         %lu\r\n"
        "  Min Stack Size:       %u bytes\r\n"
        "  Max Task Name:        %u chars\r\n"
        "\r\n"
        "MEMORY HEALTH:\r\n"
        "  Heap Status:          %s\r\n"
        "  Stack Status:         %s\r\n",
        (unsigned long)xTotalHeapSize,
        (unsigned long)xUsedHeapSize,
        (unsigned long)((xUsedHeapSize * 100) / xTotalHeapSize),
        (unsigned long)xFreeHeapSize,
        (unsigned long)xMinimumEverFreeHeapSize,
        (unsigned long)uxTotalStackAllocated,
        (unsigned long)uxTotalStackUsed,
        (uxTotalStackAllocated > 0) ? (unsigned long)((uxTotalStackUsed * 100) / uxTotalStackAllocated) : 0,
        (unsigned long)(uxTotalStackAllocated - uxTotalStackUsed),
        (unsigned long)uxTaskCount,
        configMINIMAL_STACK_SIZE * sizeof(StackType_t),
        configMAX_TASK_NAME_LEN,
        (((xUsedHeapSize * 100) / xTotalHeapSize) > 90) ? "CRITICAL" :
        (((xUsedHeapSize * 100) / xTotalHeapSize) > 75) ? "WARNING" : "OK",
        (uxTotalStackAllocated > 0 && ((uxTotalStackUsed * 100) / uxTotalStackAllocated) > 90) ? "CRITICAL" :
        (uxTotalStackAllocated > 0 && ((uxTotalStackUsed * 100) / uxTotalStackAllocated) > 75) ? "WARNING" : "OK");
    
    return pdFALSE;
}

/**
 * @brief Get task stack high water mark (wrapper for safety)
 */
static UBaseType_t prvGetTaskStackHighWaterMark(TaskHandle_t xTask)
{
    if (xTask != NULL) {
        return uxTaskGetStackHighWaterMark(xTask);
    }
    return 0;
}

/**
 * @brief Format stack information for display
 */
static void prvFormatStackInfo(char *pcBuffer, size_t xBufferLen, const char *pcTaskName, 
                              UBaseType_t uxStackSize, UBaseType_t uxHighWaterMark)
{
    UBaseType_t uxUsed = uxStackSize - uxHighWaterMark;
    UBaseType_t uxUsagePercent = (uxUsed * 100) / uxStackSize;
    
    snprintf(pcBuffer, xBufferLen,
        "%-15s: %4lu/%4lu bytes (%3lu%%) - %s\r\n",
        pcTaskName,
        (unsigned long)uxUsed,
        (unsigned long)uxStackSize,
        (unsigned long)uxUsagePercent,
        (uxUsagePercent > 90) ? "CRITICAL" :
        (uxUsagePercent > 75) ? "WARNING" : "OK");
}

/**
 * @brief Show stack overflow detection history and statistics
 */
static BaseType_t prvStackOverflowInfoCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
    (void)pcCommandString;
    
    uint32_t overflowCount = 0;
    char lastTaskName[configMAX_TASK_NAME_LEN + 1] = {0};
    uint32_t lastTaskNumber = 0;
    
    /* Get stack overflow information */
    vGetStackOverflowInfo(&overflowCount, lastTaskName, sizeof(lastTaskName), &lastTaskNumber);
    
    snprintf(pcWriteBuffer, xWriteBufferLen,
        "Stack Overflow Detection Report:\r\n"
        "================================\r\n"
        "Total Overflows Detected:   %lu\r\n"
        "Last Overflow Task:         %s\r\n"
        "Last Overflow Task Number:  %lu\r\n"
        "Detection Method:           FreeRTOS Hook (Method 2)\r\n"
        "Stack Check Enabled:        %s\r\n"
        "\r\n"
        "Status: %s\r\n"
        "\r\n"
        "Note: If overflows detected > 0, system was reset after detection.\r\n"
        "Use 'stack-check' to identify tasks at risk of overflow.\r\n",
        (unsigned long)overflowCount,
        (strlen(lastTaskName) > 0) ? lastTaskName : "None",
        (unsigned long)lastTaskNumber,
        (configCHECK_FOR_STACK_OVERFLOW == 2) ? "Yes (Method 2)" :
        (configCHECK_FOR_STACK_OVERFLOW == 1) ? "Yes (Method 1)" : "No",
        (overflowCount > 0) ? "OVERFLOW DETECTED - SYSTEM WAS RESET" : "No overflows detected");
    
    return pdFALSE;
}
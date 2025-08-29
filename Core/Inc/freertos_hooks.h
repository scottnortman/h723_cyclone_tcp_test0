/**
 * @file freertos_hooks.h
 * @brief FreeRTOS Hook Functions Header
 * 
 * This header provides the interface for FreeRTOS hook functions
 * and stack overflow tracking utilities.
 */

#ifndef FREERTOS_HOOKS_H
#define FREERTOS_HOOKS_H

#include "FreeRTOS.h"
#include "task.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Get stack overflow statistics
 * 
 * @param pxOverflowCount Pointer to store overflow count (can be NULL)
 * @param pcLastTaskName Buffer to store last overflow task name (can be NULL)
 * @param xBufferSize Size of the task name buffer
 * @param pxLastTaskNumber Pointer to store last overflow task number (can be NULL)
 */
void vGetStackOverflowInfo(uint32_t *pxOverflowCount, 
                          char *pcLastTaskName, 
                          size_t xBufferSize,
                          uint32_t *pxLastTaskNumber);

/**
 * @brief Reset stack overflow statistics
 */
void vResetStackOverflowInfo(void);

#ifdef __cplusplus
}
#endif

#endif /* FREERTOS_HOOKS_H */
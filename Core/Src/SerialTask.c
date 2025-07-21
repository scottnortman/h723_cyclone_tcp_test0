/* SerialTask.c */
#include "SerialTask.h"
#include "FreeRTOS_CLI.h"  /* for configCOMMAND_INT_MAX_* */
#include <string.h>

/* External HAL handles */
extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef  hdma_usart3_rx;

/* Stream buffer handles */
static StreamBufferHandle_t xSerialRxStream = NULL;
static StreamBufferHandle_t xSerialTxStream = NULL;

/* DMA circular buffer for RX */
static volatile uint8_t dma_buf[SERIAL_TASK_RX_BUFFER_SIZE];
static volatile size_t dma_head = 0;

/* Mutex for UART TX (blocking HAL transmit) */
static SemaphoreHandle_t xUSART3TxMutex = NULL;

/* Internal RX/TX tasks */
static void vSerialRxTask(void *pvParameters);
static void vSerialTxTask(void *pvParameters);
#ifdef SERIAL_TASK_LOOPBACK
static void vSerialLoopbackTask(void *pvParameters);
#endif

void vSerialTaskInit(UBaseType_t xTxPriority, UBaseType_t xRxPriority)
{
    /* Create byte-stream buffers */
    xSerialRxStream = xStreamBufferCreate(
        SERIAL_TASK_RX_BUFFER_SIZE,
        SERIAL_TASK_TRIGGER_LEVEL);
    configASSERT(xSerialRxStream != NULL);

    xSerialTxStream = xStreamBufferCreate(
        SERIAL_TASK_TX_BUFFER_SIZE,
        SERIAL_TASK_TRIGGER_LEVEL);
    configASSERT(xSerialTxStream != NULL);

    /* Mutex for exclusive UART access */
    xUSART3TxMutex = xSemaphoreCreateMutex();
    configASSERT(xUSART3TxMutex != NULL);

    /* Start the RX and TX tasks */
    BaseType_t ret;
    ret = xTaskCreate(
        vSerialRxTask,
        "SerialRx",
        configMINIMAL_STACK_SIZE,
        NULL,
        xRxPriority,
        NULL);
    configASSERT(ret == pdPASS);

    ret = xTaskCreate(
        vSerialTxTask,
        "SerialTx",
        configMINIMAL_STACK_SIZE,
        NULL,
        xTxPriority,
        NULL);
    configASSERT(ret == pdPASS);

#ifdef SERIAL_TASK_LOOPBACK
    /* Automatically start loopback test if enabled */
    vSerialLoopbackTestStart(xRxPriority);
#endif
}

#ifdef SERIAL_TASK_LOOPBACK
void vSerialLoopbackTestStart(UBaseType_t uxPriority)
{
    BaseType_t ret = xTaskCreate(
        vSerialLoopbackTask,
        "SerialLoop",
        configMINIMAL_STACK_SIZE,
        NULL,
        uxPriority,
        NULL);
    configASSERT(ret == pdPASS);
}
#endif

StreamBufferHandle_t xSerialTaskGetRxStreamHandle(void)
{
    return xSerialRxStream;
}

StreamBufferHandle_t xSerialTaskGetTxStreamHandle(void)
{
    return xSerialTxStream;
}

void vSerialPutChar(char c)
{
    xStreamBufferSend(xSerialTxStream, &c, 1, 0);
}

void vSerialPutString(const char * buf, size_t len)
{
    xStreamBufferSend(xSerialTxStream, buf, len, 0);
}

int __io_putchar(int ch)
{
    char c = (char) ch;
    xStreamBufferSend(xSerialTxStream, &c, 1, 0);
    return ch;
}

/* Task: Drain TX stream and physically send bytes */
static void vSerialTxTask(void *pvParameters)
{
    uint8_t c;
    for (;;)
    {
        if (xStreamBufferReceive(xSerialTxStream, &c, 1, portMAX_DELAY) > 0)
        {
            if (xSemaphoreTake(xUSART3TxMutex, portMAX_DELAY) == pdTRUE)
            {
                HAL_UART_Transmit(&huart3, &c, 1, HAL_MAX_DELAY);
                xSemaphoreGive(xUSART3TxMutex);
            }
        }
    }
}

/* UART3 ISR: push DMA-received bytes into RX stream */
void USART3_IRQHandler(void)
{
    BaseType_t xWoken = pdFALSE;
    size_t head = SERIAL_TASK_RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart3_rx);
    size_t cnt  = (head >= dma_head)
                ? (head - dma_head)
                : (SERIAL_TASK_RX_BUFFER_SIZE - dma_head + head);

    if (cnt > 0)
    {
        for (size_t i = 0; i < cnt; ++i)
        {
            uint8_t c = dma_buf[(dma_head + i) % SERIAL_TASK_RX_BUFFER_SIZE];
            xStreamBufferSendFromISR(xSerialRxStream, &c, 1, &xWoken);
        }
        dma_head = head;
    }
    __HAL_UART_CLEAR_IDLEFLAG(&huart3);
    HAL_UART_IRQHandler(&huart3);
    portYIELD_FROM_ISR(xWoken);
}

/* Setup DMA RX and IDLE interrupt */
static void vSerialRxTask(void *pvParameters)
{
    HAL_UART_Receive_DMA(&huart3, (uint8_t *)dma_buf, SERIAL_TASK_RX_BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(&hdma_usart3_rx, DMA_IT_TC | DMA_IT_HT);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);

    /* This task simply sleeps; ISR does the work */
    for (;;)
    {
        vTaskDelay(portMAX_DELAY);
    }
}

#ifdef SERIAL_TASK_LOOPBACK
/* Loopback test task: echoes RX bytes back on TX */
static void vSerialLoopbackTask(void *pvParameters)
{
    uint8_t c;
    for (;;)
    {
        if (xStreamBufferReceive(xSerialRxStream, &c, 1, portMAX_DELAY) > 0)
        {
            xStreamBufferSend(xSerialTxStream, &c, 1, portMAX_DELAY);
        }
    }
}
#endif

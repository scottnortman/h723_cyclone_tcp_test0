/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "dma.h"
#include "eth.h"
#include "memorymap.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdlib.h>
#include <stdio.h>
#include <inttypes.h>

//#include "UARTCommandConsole.h"



#include "SerialTask.h"
#include "TelnetTask.h"
#include "CommandConsoleTask.h"
#include "Sample-CLI-commands.h"

#include "core/net.h"
#include "drivers/mac/stm32h7xx_eth_driver.h"
#include "drivers/phy/lan8742_driver.h"
#include "dhcp/dhcp_client.h"
#include "ipv6/slaac.h"
#include "mdns/mdns_responder.h"
#include "http/http_client.h"
#include "icmp.h"
#include "debug.h"

#include "udpard.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

//Ethernet interface configuration
#define APP_IF_NAME "eth0"
#define APP_HOST_NAME "http-client-demo"
#define APP_MAC_ADDR "00-AB-CD-EF-07-43"

//#define APP_USE_DHCP_CLIENT ENABLED
#define APP_USE_DHCP_CLIENT DISABLED
#define APP_IPV4_HOST_ADDR "192.168.0.20"
#define APP_IPV4_SUBNET_MASK "255.255.255.0"
#define APP_IPV4_DEFAULT_GATEWAY "192.168.0.254"
#define APP_IPV4_PRIMARY_DNS "8.8.8.8"
#define APP_IPV4_SECONDARY_DNS "8.8.4.4"

#define APP_USE_SLAAC DISABLED
//#define APP_IPV6_LINK_LOCAL_ADDR "fe80::743"
//#define APP_IPV6_PREFIX "2001:db8::"
//#define APP_IPV6_PREFIX_LENGTH 64
//#define APP_IPV6_GLOBAL_ADDR "2001:db8::743"
//#define APP_IPV6_ROUTER "fe80::1"
//#define APP_IPV6_PRIMARY_DNS "2001:4860:4860::8888"
//#define APP_IPV6_SECONDARY_DNS "2001:4860:4860::8844"

//Application configuration
#define APP_HTTP_SERVER_NAME "www.httpbin.org"
#define APP_HTTP_SERVER_PORT 80
#define APP_HTTP_URI "/anything"

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

__IO uint32_t BspButtonState = BUTTON_RELEASED;

/* USER CODE BEGIN PV */

//Global variables
DhcpClientSettings dhcpClientSettings;
DhcpClientContext dhcpClientContext;
SlaacSettings slaacSettings;
SlaacContext slaacContext;
MdnsResponderSettings mdnsResponderSettings;
MdnsResponderContext mdnsResponderContext;
HttpClientContext httpClientContext;

//static xComPortHandle comPortHandle = NULL;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MPU_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */


void initTask(void);
error_t httpClientTest(void);
void userTask(void *param);
void ledTask(void *param);


#ifdef configUSE_TICK_HOOK
static volatile uint64_t xTickCount64 = 0;
void vApplicationTickHook( void ){
	xTickCount64++;
}
uint64_t xTaskGetTickCount64(void){
	uint64_t value;
	taskENTER_CRITICAL();
	value = xTickCount64;
	taskEXIT_CRITICAL();
	return value;
}
#endif // configUSE_TICK_HOOK

/**
 * @brief User task
 * @param[in] param Unused parameter
 **/

void userTask(void *param)
{
   //Endless loop
   while(1)
   {
      //User button pressed?
      if(BSP_PB_GetState(BUTTON_USER))
      {
         //HTTP client test routine
         httpClientTest();

         //Wait for the user button to be released
         while(BSP_PB_GetState(BUTTON_USER));
      }

      //Loop delay
      osDelayTask(100);
   }
}

/**
 * @brief LED task
 * @param[in] param Unused parameter
 **/

void pvGreenLEDTask(void *param)
{
   //Endless loop
   while(1)
   {
      BSP_LED_Toggle(LED_GREEN);
      osDelayTask(100);
   }
}

void pvRedLEDTask(void *pvParams){
	   while(1)
	   {
	      BSP_LED_Toggle(LED_RED);
	      osDelayTask(200);
	      //printf("%0.3f\r\n", (float)xTaskGetTickCount64()/(float)configTICK_RATE_HZ );
	   }
}




/**
 * @brief Initialization task
 * @param[in] param Unused parameter
 **/

void initTask(void)
{
   error_t error;

   NetInterface *interface;
   MacAddr macAddr;

   #if (APP_USE_DHCP_CLIENT == DISABLED)
      Ipv4Addr ipv4Addr;
   #endif

   //TCP/IP stack initialization
   error = netInit();
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Initialized TCP/IP Stack...\r\n");

   //Configure the first Ethernet interface
   interface = &netInterface[0];

   //Set interface name
   error = netSetInterfaceName(interface, APP_IF_NAME);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Set interface name to [%s]...\r\n", APP_IF_NAME);

   //Set host name
   error = netSetHostname(interface, APP_HOST_NAME);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Set hostname to [%s]...\r\n", APP_HOST_NAME);

   //Set host MAC address
   macStringToAddr(APP_MAC_ADDR, &macAddr);
   error = netSetMacAddr(interface, &macAddr);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Set MAC address to [%s]...\r\n", APP_MAC_ADDR);

   //Select the relevant network adapter
   netSetDriver(interface, &stm32h7xxEthDriver);
   error = netSetPhyDriver(interface, &lan8742PhyDriver);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Set PHY driver...\r\n");

   //Initialize network interface
   error = netConfigInterface(interface);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Configured network interface...\r\n");

   #if (IPV4_SUPPORT == ENABLED)

	   #if (APP_USE_DHCP_CLIENT == ENABLED)

	   //Get default settings
	   dhcpClientGetDefaultSettings(&dhcpClientSettings);
	   //Set the network interface to be configured by DHCP
	   dhcpClientSettings.interface = interface;
	   //Disable rapid commit option
	   dhcpClientSettings.rapidCommit = FALSE;

	   //DHCP client initialization
	   error = dhcpClientInit(&dhcpClientContext, &dhcpClientSettings);
	   configASSERT(NO_ERROR==error);
	   TRACE_INFO("Initializes DHCP client...\r\n");

	   error = dhcpClientStart(&dhcpClientContext);
	   configASSERT(NO_ERROR==error);
	   TRACE_INFO("Started DHCP client...\r\n");

   	   #else

		   //Set IPv4 host address manually....
		   ipv4StringToAddr(APP_IPV4_HOST_ADDR, &ipv4Addr);
		   ipv4SetHostAddr(interface, ipv4Addr);

		   //Set subnet mask
		   ipv4StringToAddr(APP_IPV4_SUBNET_MASK, &ipv4Addr);
		   ipv4SetSubnetMask(interface, ipv4Addr);

		   //Set default gateway
		   ipv4StringToAddr(APP_IPV4_DEFAULT_GATEWAY, &ipv4Addr);
		   ipv4SetDefaultGateway(interface, ipv4Addr);

		   //Set primary and secondary DNS servers
		   ipv4StringToAddr(APP_IPV4_PRIMARY_DNS, &ipv4Addr);
		   ipv4SetDnsServer(interface, 0, ipv4Addr);
		   ipv4StringToAddr(APP_IPV4_SECONDARY_DNS, &ipv4Addr);
		   ipv4SetDnsServer(interface, 1, ipv4Addr);

	   #endif

   #endif

   //Get default settings
   mdnsResponderGetDefaultSettings(&mdnsResponderSettings);
   //Underlying network interface
   mdnsResponderSettings.interface = &netInterface[0];

   //mDNS responder initialization
   error = mdnsResponderInit(&mdnsResponderContext, &mdnsResponderSettings);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Initialized mDNS responder...\r\n");

   //Set mDNS hostname
   error = mdnsResponderSetHostname(&mdnsResponderContext, APP_HOST_NAME);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("mDNS set hostname...\r\n");

   //Start mDNS responder
   error = mdnsResponderStart(&mdnsResponderContext);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Started mDNS responder...\r\n");

   error = icmpEnableEchoRequests(interface, TRUE);
   configASSERT(NO_ERROR==error);
   TRACE_INFO("Enabled ICMP requests...\r\n");

} // initTask

/**
 * @brief HTTP client test routine
 * @return Error code
 **/

error_t httpClientTest(void)
{
   error_t error;
   size_t length;
   uint_t status;
   const char_t *value;
   IpAddr ipAddr;
   char_t buffer[128];

   //Initialize HTTP client context
   httpClientInit(&httpClientContext);

   //Start of exception handling block
   do
   {
      //Debug message
      TRACE_INFO("\r\n\r\nResolving server name...\r\n");

      //Resolve HTTP server name
      error = getHostByName(NULL, APP_HTTP_SERVER_NAME, &ipAddr, 0);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to resolve server name!\r\n");
         break;
      }

      //Select HTTP protocol version
      error = httpClientSetVersion(&httpClientContext, HTTP_VERSION_1_1);
      //Any error to report?
      if(error)
         break;

      //Set timeout value for blocking operations
      error = httpClientSetTimeout(&httpClientContext, 20000);
      //Any error to report?
      if(error)
         break;

      //Debug message
      TRACE_INFO("Connecting to HTTP server %s...\r\n",
         ipAddrToString(&ipAddr, NULL));

      //Connect to the HTTP server
      error = httpClientConnect(&httpClientContext, &ipAddr,
         APP_HTTP_SERVER_PORT);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to connect to HTTP server!\r\n");
         break;
      }

      //Create an HTTP request
      httpClientCreateRequest(&httpClientContext);
      httpClientSetMethod(&httpClientContext, "POST");
      httpClientSetUri(&httpClientContext, APP_HTTP_URI);

      //Set the hostname and port number of the resource being requested
      httpClientSetHost(&httpClientContext, APP_HTTP_SERVER_NAME,
         APP_HTTP_SERVER_PORT);

      //Set query string
      httpClientAddQueryParam(&httpClientContext, "param1", "value1");
      httpClientAddQueryParam(&httpClientContext, "param2", "value2");

      //Add HTTP header fields
      httpClientAddHeaderField(&httpClientContext, "User-Agent", "Mozilla/5.0");
      httpClientAddHeaderField(&httpClientContext, "Content-Type", "text/plain");
      httpClientAddHeaderField(&httpClientContext, "Transfer-Encoding", "chunked");

      //Send HTTP request header
      error = httpClientWriteHeader(&httpClientContext);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to write HTTP request header!\r\n");
         break;
      }

      //Send HTTP request body
      error = httpClientWriteBody(&httpClientContext, "Hello World!", 12,
         NULL, 0);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to write HTTP request body!\r\n");
         break;
      }

      //Receive HTTP response header
      error = httpClientReadHeader(&httpClientContext);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to read HTTP response header!\r\n");
         break;
      }

      //Retrieve HTTP status code
      status = httpClientGetStatus(&httpClientContext);
      //Debug message
      TRACE_INFO("HTTP status code: %u\r\n", status);

      //Retrieve the value of the Content-Type header field
      value = httpClientGetHeaderField(&httpClientContext, "Content-Type");

      //Header field found?
      if(value != NULL)
      {
         //Debug message
         TRACE_INFO("Content-Type header field value: %s\r\n", value);
      }
      else
      {
         //Debug message
         TRACE_INFO("Content-Type header field not found!\r\n");
      }

      //Receive HTTP response body
      while(!error)
      {
         //Read data
         error = httpClientReadBody(&httpClientContext, buffer,
            sizeof(buffer) - 1, &length, 0);

         //Check status code
         if(!error)
         {
            //Properly terminate the string with a NULL character
            buffer[length] = '\0';
            //Dump HTTP response body
            TRACE_INFO("%s", buffer);
         }
      }

      //Terminate the HTTP response body with a CRLF
      TRACE_INFO("\r\n");

      //Any error to report?
      if(error != ERROR_END_OF_STREAM)
         break;

      //Close HTTP response body
      error = httpClientCloseBody(&httpClientContext);
      //Any error to report?
      if(error)
      {
         //Debug message
         TRACE_INFO("Failed to read HTTP response trailer!\r\n");
         break;
      }

      //Gracefully disconnect from the HTTP server
      httpClientDisconnect(&httpClientContext);

      //Debug message
      TRACE_INFO("Connection closed\r\n");

      //End of exception handling block
   } while(0);

   //Release HTTP client context
   httpClientDeinit(&httpClientContext);

   //Return status code
   return error;

} // httpClientTest

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
//int _write(int le, char *ptr, int len){
//	HAL_UART_Transmit(&huart3, (uint8_t *)ptr, len, 100);
//	return len;
//}



/**
 * @brief Display the contents of an array
 * @param[in] stream Pointer to a FILE object that identifies an output stream
 * @param[in] prepend String to prepend to the left of each line
 * @param[in] data Pointer to the data array
 * @param[in] length Number of bytes to display
 **/

void debugDisplayArray(FILE *stream,
   const char_t *prepend, const void *data, size_t length)
{
   uint_t i;

   for(i = 0; i < length; i++)
   {
      //Beginning of a new line?
      if((i % 16) == 0)
         fprintf(stream, "%s", prepend);
      //Display current data byte

      fprintf(stream, "%02" PRIX8 " ", *((uint8_t *) data + i));
      //End of current line?
      if((i % 16) == 15 || i == (length - 1))
         fprintf(stream, "\r\n");
   }
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	error_t error;
	OsTaskId taskId;
	OsTaskParameters taskParams;

  /* USER CODE END 1 */

  /* MPU Configuration--------------------------------------------------------*/
  MPU_Config();

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ETH_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */

  taskParams = OS_TASK_DEFAULT_PARAMS;
  taskParams.stackSize = 128;
  taskParams.priority = OS_TASK_PRIORITY_NORMAL+1;

  taskId = osCreateTask( "GRN", pvGreenLEDTask, NULL, &taskParams);
  //taskId = osCreateTask( "RED", pvRedLEDTask, NULL, &taskParams );

  BaseType_t ret;
  ret = xTaskCreate( pvRedLEDTask, "RED", configMINIMAL_STACK_SIZE, NULL, 4, NULL );
  configASSERT(ret == pdPASS);

  //xUARTCommandConsoleStart( configUART_COMMAND_CONSOLE_STACK, 2 );
  //vRegisterSampleCLICommands();



  //xSerialPortInitMinimal(115200);

  vSerialTaskInit( tskIDLE_PRIORITY+1, tskIDLE_PRIORITY+1 );


  xTelnetTaskStart( tskIDLE_PRIORITY+1 );

  //vCommandConsoleInit(xSerialTaskGetRxStreamHandle(), xSerialTaskGetTxStreamHandle(), 0, 0);
  vCommandConsoleInit(xTelnetTaskGetRxStreamHandle(), xTelnetTaskGetTxStreamHandle(), 0, 0);


  vRegisterSampleCLICommands();







  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Initialize leds */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_RED);

  /* Initialize USER push-button, will be used to trigger an interrupt each time it's pressed.*/
  BSP_PB_Init(BUTTON_USER, BUTTON_MODE_EXTI);

  /* USER CODE BEGIN BSP */

  //comPortHandle = xSerialPortInitMinimal(115200,  configCOMMAND_INT_MAX_OUTPUT_SIZE);
  //xUARTCommandConsoleSetPort( comPortHandle );



  /* -- Sample board code to send message over COM1 port ---- */
  printf("Welcome to STM32 world !\r\n");

  /* -- Sample board code to switch on leds ---- */
  BSP_LED_On(LED_GREEN);
  BSP_LED_On(LED_YELLOW);
  BSP_LED_On(LED_RED);

  initTask();



  //error = httpClientTest();



  /* USER CODE END BSP */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {

    /* -- Sample board code for User push-button in interrupt mode ---- */
    if (BspButtonState == BUTTON_PRESSED)
    {
      /* Update button state */
      BspButtonState = BUTTON_RELEASED;
      /* -- Sample board code to toggle leds ---- */
      BSP_LED_Toggle(LED_GREEN);
      BSP_LED_Toggle(LED_YELLOW);
      BSP_LED_Toggle(LED_RED);

      /* ..... Perform your action ..... */
    }
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 275;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

 /* MPU Configuration */

void MPU_Config(void)
{
  MPU_Region_InitTypeDef MPU_InitStruct = {0};

  /* Disables the MPU */
  HAL_MPU_Disable();

  /** Initializes and configures the Region and the memory to be protected
  */
  MPU_InitStruct.Enable = MPU_REGION_ENABLE;
  MPU_InitStruct.Number = MPU_REGION_NUMBER0;
  MPU_InitStruct.BaseAddress = 0x0;
  MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
  MPU_InitStruct.SubRegionDisable = 0x87;
  MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
  MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
  MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;
  MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
  MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
  MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;

  HAL_MPU_ConfigRegion(&MPU_InitStruct);
  /* Enables the MPU */
  HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);

}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM23 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM23)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

/**
  * @brief BSP Push Button callback
  * @param Button Specifies the pressed button
  * @retval None
  */
void BSP_PB_Callback(Button_TypeDef Button)
{
  if (Button == BUTTON_USER)
  {
    BspButtonState = BUTTON_PRESSED;
  }
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

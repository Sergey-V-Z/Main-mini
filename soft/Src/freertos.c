/* USER CODE BEGIN Header */
/**
******************************************************************************
* File Name          : freertos.c
* Description        : Code for freertos applications
******************************************************************************
* @attention
*
* <h2><center>&copy; Copyright (c) 2021 STMicroelectronics.
* All rights reserved.</center></h2>
*
* This software component is licensed by ST under Ultimate Liberty license
* SLA0044, the "License"; You may not use this file except in compliance with
* the License. You may obtain a copy of the License at:
*                             www.st.com/SLA0044
*
******************************************************************************
https://github.com/ADElectronics/STM32-FreeModbus-Example
*/
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ethernetif.h"
#include "lwip/timeouts.h"
#include "lwip.h"
#include "api.h"
#include <string.h>
#include "mb.h"
#include "mbport.h"
#include "user_mb_app_m.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
extern uint16_t usMRegInBuf[MB_MASTER_TOTAL_SLAVE_NUM][M_REG_INPUT_NREGS];
extern uint16_t   usMRegHoldBuf[MB_MASTER_TOTAL_SLAVE_NUM][M_REG_HOLDING_NREGS];
eMBMasterReqErrCode req_M;

extern BOOL xNeedPoll;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim13;
extern TIM_HandleTypeDef htim14;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

//eth
extern struct netif gnetif;
char            *pIP;
uint32_t        ip = 0;

//хранит команду из TCP
uint8_t         input_data[100] = {0};
uint8_t 		response[100] = {0};

/* USER CODE END Variables */
osThreadId MainTaskHandle;
osThreadId ModBus1TaskHandle;
osThreadId ModBus2TaskHandle;
osThreadId TCP_ServerHandle;
osSemaphoreId InDataTCPHandle;
osSemaphoreId ModBusEndHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void mainTask(void const * argument);
void modbus1Task(void const * argument);
void modbus2Task(void const * argument);
void tcp_server(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
  {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
    *ppxIdleTaskStackBuffer = &xIdleStack[0];
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
    /* place for user code */
  }                   
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* definition and creation of InDataTCP */
  osSemaphoreDef(InDataTCP);
  InDataTCPHandle = osSemaphoreCreate(osSemaphore(InDataTCP), 1);

  /* definition and creation of ModBusEnd */
  osSemaphoreDef(ModBusEnd);
  ModBusEndHandle = osSemaphoreCreate(osSemaphore(ModBusEnd), 1);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of MainTask */
  osThreadDef(MainTask, mainTask, osPriorityLow, 0, 256);
  MainTaskHandle = osThreadCreate(osThread(MainTask), NULL);

  /* definition and creation of ModBus1Task */
  osThreadDef(ModBus1Task, modbus1Task, osPriorityLow, 0, 256);
  ModBus1TaskHandle = osThreadCreate(osThread(ModBus1Task), NULL);

  /* definition and creation of ModBus2Task */
  osThreadDef(ModBus2Task, modbus2Task, osPriorityLow, 0, 256);
  ModBus2TaskHandle = osThreadCreate(osThread(ModBus2Task), NULL);

  /* definition and creation of TCP_Server */
  osThreadDef(TCP_Server, tcp_server, osPriorityLow, 0, 256);
  TCP_ServerHandle = osThreadCreate(osThread(TCP_Server), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_mainTask */
/**
* @brief  Function implementing the MainTask thread.
* @param  argument: Not used 
* @retval None
*/
/* USER CODE END Header_mainTask */
void mainTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN mainTask */
    
    //printf("Start StartTask02\r\n");   
    
//    struct netconn *nc;
//    struct netbuf *nb;
//    volatile err_t res;
//    uint16_t len;
//    ip_addr_t local_ip;
//    ip_addr_t remote_ip;
//    char buffer[512];// = pvPortMalloc(512);
//    
//    while(gnetif.ip_addr.addr == 0) osDelay(10);
//    //printf("IP: %s\r\n", ip4addr_ntoa(&gnetif.ip_addr));
//    
//    local_ip = gnetif.ip_addr;
//    ip4addr_aton("192.168.20.108", &remote_ip);
//    
//    nc = netconn_new(NETCONN_TCP);
//    if(nc == NULL){
//      //printf("new error\r\n");
//      while(1)osDelay(1);
//    }
//    
//    res = netconn_bind(nc, &local_ip, 0);
//    if(res != NULL){
//      //printf("bind error\r\n");
//      while(1)osDelay(1);
//    }
//    
//    res = netconn_connect(nc, &remote_ip, 80);
//    if(res != NULL){
//      //printf("conn error\r\n");
//      while(1)osDelay(1);
//    }
//    
//    sprintf(buffer, "from stm32\r\n");
//    sprintf(buffer, ip4addr_ntoa(&gnetif.ip_addr));
//    res = netconn_write(nc, &buffer, strlen(buffer), NETCONN_COPY);
//    if(res != NULL){
//      //printf("write error\r\n");
//      while(1)osDelay(1);
//    }
//    
//    res = netconn_recv(nc, &nb);
//    if(res != NULL){
//      //printf("reciv error\r\n");
//      while(1)osDelay(1);
//    }
//    len = netbuf_len(nb);
//    netbuf_copy(nb, buffer, len);
//    netbuf_delete(nb);
//    //printf("Recive %d bytes:\r\n%s\r\n",len, buffer);
    /* Infinite loop */
    for(;;)
      {
//        //ethernetif_input(&gnetif);
//        //sys_check_timeouts();
//        res = netconn_recv(nc, &nb);
//        if(res != NULL){
//          //printf("reciv error\r\n");
//          while(1)osDelay(1);
//        }
//        len = netbuf_len(nb);
//        if(len != 0){
//          netbuf_copy(nb, buffer, len);
//          netbuf_delete(nb);
//          buffer[len] = 0;
//          //printf("Recive %d bytes:\r\n%s\r\n",len, buffer);  
//          //HAL_UART_Transmit(&huart3, buffer, len, 100);
//          netconn_write(nc, &buffer, strlen(buffer), NETCONN_COPY);
//        }
//        
        osDelay(10);
        
      }
  /* USER CODE END mainTask */
}

/* USER CODE BEGIN Header_modbus1Task */
/**
* @brief Function implementing the ModBus1Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_modbus1Task */
void modbus1Task(void const * argument)
{
  /* USER CODE BEGIN modbus1Task */
   eMBMasterInit( MB_RTU, &huart1, 115200, &htim13 );
   eMBMasterEnable( );
   xNeedPoll = TRUE;
   osSemaphoreWait(InDataTCPHandle,1);
   int32_t SemRet = 0;
   /* Infinite loop */
   for(;;)
   {
      eMBMasterPoll();
      osDelay(1);
//      if(xNeedPoll)
//      {
//         req_M = eMBMasterReqReadHoldingRegister(0x03, 0, 2, 2);
//         xNeedPoll = FALSE;
//      } 
      //ждем симафора
      SemRet = osSemaphoreWait(InDataTCPHandle,1);
      if(SemRet != osErrorOS)
      {
         switch(input_data[0])
         {
           case 1: // read
            {
               for(int i = 3, a = 0; a < input_data[1]; ++i, ++a)//отправить запросы в 485
               {
                  while(!xNeedPoll)
                  {}                                    //UCHAR ucSndAddr, USHORT usRegAddr, USHORT usNRegs, LONG lTimeOut
                  req_M = eMBMasterReqReadHoldingRegister(input_data[i], input_data[2], 1, 2000);
                  xNeedPoll = FALSE;  
               }
//               for(int i = 0; i < input_data[1]; ++i)// сохранить в буфер отправки ответы 
//               {
//                  
//               }  
//               req_M = eMBMasterWaitRequestFinish();
//               while(eMBMasterWaitRequestFinish() != MB_MRE_NO_ERR)
//               {}
               //выдать симафор 
//               osSemaphoreRelease(ModBusEndHandle);
               break;
            }
           case 2: // write
            {
               
               for(int i = 3; i < input_data[1]*2; ++i)//отправить запросы в 485
               {
                  while(!xNeedPoll)
                  {}
                  req_M = eMBMasterReqWriteHoldingRegister(input_data[i], input_data[2], input_data[i+1], 2);
                  xNeedPoll = FALSE;
                  ++i;
                  // сохранить в буфер отправки ответы req_M
               }    
//               while(!xNeedPoll)
//               {}
//               //выдать симафор 
//               osSemaphoreRelease(ModBusEndHandle);
               break;
            }
           default:
            {
               //netconn_write(newconn,0,1,NETCONN_COPY);
               break;
            }
            
         }
      }
      
   }
  /* USER CODE END modbus1Task */
}

/* USER CODE BEGIN Header_modbus2Task */
/**
* @brief Function implementing the ModBus2Task thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_modbus2Task */
void modbus2Task(void const * argument)
{
  /* USER CODE BEGIN modbus2Task */
    /* Infinite loop */
    for(;;)
      {
        osDelay(1);
      }
  /* USER CODE END modbus2Task */
}

/* USER CODE BEGIN Header_tcp_server */
/**
* @brief Function implementing the TCP_Server thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_tcp_server */
void tcp_server(void const * argument)
{
  /* USER CODE BEGIN tcp_server */
  /* Infinite loop */
 //TCP connection vars
   err_t                err, accept_err;
   struct netbuf        buffer;
   struct netconn 	connection , newconnection;
   struct netbuf 	*buf = &buffer; //bufferized input data
   struct netconn 	*conn = &connection, *newconn = &newconnection; //contains info about connection inc. type, port, buf pointers etc.
   void 		*in_data = NULL;
   uint16_t 		data_size = 0;
   osSemaphoreWait(ModBusEndHandle,1000);
   int32_t SemRet = 0;
   //sizeH = xPortGetMinimumEverFreeHeapSize();
  /* Infinite loop */
  for(;;)
  {
    while(gnetif.ip_addr.addr == 0) osDelay(10);
    
    if((gnetif.ip_addr.addr != 0) && (gnetif.ip_addr.addr != ip)){
       ip = gnetif.ip_addr.addr;       
       pIP = ip4addr_ntoa(&gnetif.ip_addr);
    }
     
     conn = netconn_new(NETCONN_TCP);
     if (conn!=NULL)
     {
        err = netconn_bind(conn,NULL,81);//assign port number to connection
        if (err==ERR_OK)
        {
           netconn_listen(conn);//set port to listening mode
           while(1)
           {
              accept_err=netconn_accept(conn,&newconn);//suspend until new connection 
              if (accept_err==ERR_OK)
              {
                 while (netconn_recv(newconn,&buf)==ERR_OK)//suspend until data received
                 {
                    do
                    {
                       netbuf_data(buf,&in_data,&data_size);//get pointer and data size of the buffer
                       memcpy((void*)input_data,in_data,data_size);
                       
                       //выдать симафор для modbus
                       osSemaphoreRelease(InDataTCPHandle);
                       
                       // ожидание симафора
                      SemRet = osSemaphoreWait(ModBusEndHandle,1000);
                       //проверить если семафор не пришел то вернуть ощибку
                      if(SemRet != 0)
                      {
                         response[0] = -1;
                         xNeedPoll = TRUE;
                      }else{
                         //обработать данные из modbus
                         for(volatile int i = 3, a = 0; a < input_data[1]; ++i, ++a)
                         {
                            response[a+1] = (uint8_t)(usMRegHoldBuf[input_data[i]-1][input_data[2]]);
                         }
                      }
                       netconn_write(newconn,response,input_data[1]+1,NETCONN_COPY);
                       __ASM("NOP");
                    } while (netbuf_next(buf) >= 0);
                    netbuf_delete(buf);
                 }
                 netconn_close(newconn);
                 netconn_delete(newconn);
              } else netconn_delete(newconn);
              osDelay(20);
           }
        }
     }
    osDelay(1000);
  }
  /* USER CODE END tcp_server */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/

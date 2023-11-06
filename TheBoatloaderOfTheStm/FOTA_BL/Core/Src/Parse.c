/*
 * Parse.c
 *
 *  Created on: Oct 22, 2023
 *      Author: MOUSTAFA
 */

#include "Parse.h"


static void OTA_vidSetHighAddress(void);
static void OTA_vidParseRecord();
static uint8_t getHex(uint8_t Copy_u8Asci);
static void OTA_vidRunAppCode(void);
static void OTA_vidCharReceived(uint8_t rec);

volatile uint8_t g_recieveBuffer[100]; /* Array to store the received line */
volatile uint8_t g_recieveCounter = 0; /* Counter for received line chars */
volatile uint16_t g_data[100] = {0};   /* Array to store the data of line to pass it to Flash Writer */
volatile uint32_t g_address;           /* Pointer to store the address in to write */



volatile uint8_t g_lineReceivedFlag = FALSE;  /* Flag to check if the NodeMCU finished sending a complete line */
volatile uint8_t g_finishReceiveFlag = FALSE; /* Flag to check if NodeMCU finished sending the code to run application code */

uint8_t OTA_DataConfimation[4]={OTA_DATA_START_CHAR, OTA_LINE_BREAK_CHAR,OTA_DATA_END_CHAR,OTA_READ_CONFIRM_CHAR};
/* Pointer to function to run application code */
typedef void (*Function_t)(void);
Function_t addr_to_call = 0;


void FOTA_vidInit(void){
	HAL_Delay(OTA_RECIEVE_TIMEOUT_S * 1000);
	if(HAL_UART_Receive_IT(&huart1,g_recieveBuffer,1)==HAL_OK){
	}
	else {
		OTA_vidRunAppCode();
	}
}


static void OTA_vidRunAppCode(void)
{
    *((volatile uint32_t *)0xE000ED08) = 0x08002000;
    addr_to_call = *(Function_t *)(0x08002004);
    addr_to_call();
}
void FOTA_vidRun(void){
	HAL_UART_Receive_IT(&huart1,g_recieveBuffer,1);
    while (g_finishReceiveFlag == FALSE)
	    {
    	    OTA_vidCharReceived(g_recieveBuffer[g_recieveCounter]);
	        if (g_lineReceivedFlag == TRUE)
	        {
	            switch (g_recieveBuffer[g_recieveCounter])
	            {
	            case OTA_DATA_START_CHAR:
	                g_recieveCounter = 0;
	                HAL_UART_Receive(&huart1,g_recieveBuffer,22,HAL_MAX_DELAY);
	                for(int i=0;i<=56;i++){
	                		FLASH_PageErase(0x08002000+ (i *1024));
	                }
	                HAL_UART_Transmit(&huart1,&OTA_DataConfimation[0],1,HAL_MAX_DELAY);
	                break;
	            case OTA_LINE_BREAK_CHAR:
	                OTA_vidParseRecord();
	                g_recieveCounter = 0;
	                HAL_UART_Transmit(&huart1,&OTA_DataConfimation[0],1,HAL_MAX_DELAY);
	                break;
	            default:
	                break;
	            }
	            g_lineReceivedFlag = FALSE;
	        }
	    }
        HAL_UART_Abort_IT(&huart1);
	    OTA_vidRunAppCode();
}


static uint8_t getHex(uint8_t Copy_u8Asci)
{
	uint8_t Result = 0;

    /*0 ... 9*/
    if ((Copy_u8Asci >= 48) && (Copy_u8Asci <= 57))
    {
        Result = Copy_u8Asci - 48;
    }

    /*A ... F*/
    else if ((Copy_u8Asci >= 65) && (Copy_u8Asci <= 70))
    {
        Result = Copy_u8Asci - 55;
    }

    return Result;
}

static void OTA_vidParseRecord()
{
	uint8_t CC, i;
	uint8_t dataDigits[4];
	uint8_t dataCounter = 0;

    switch (getHex(g_recieveBuffer[8]))
    {

    case 4: /* Extended Linear Address Record: used to identify the extended linear address  */
        OTA_vidSetHighAddress();
        break;

    case 5: /* Start Linear Address Record: the address where the program starts to run      */
        break;

    case 0: /* Data Rrecord: used to record data, most records of HEX files are data records */

        /* Get Character Count */
        CC = (getHex(g_recieveBuffer[1]) << 4) | getHex(g_recieveBuffer[2]);

        /* Get low part of address */
        dataDigits[0] = getHex(g_recieveBuffer[3]);
        dataDigits[1] = getHex(g_recieveBuffer[4]);
        dataDigits[2] = getHex(g_recieveBuffer[5]);
        dataDigits[3] = getHex(g_recieveBuffer[6]);

        /* Set full address */
        g_address = g_address & 0xFFFF0000;
        g_address = g_address |
                    (dataDigits[3]) |
                    (dataDigits[2] << 4) |
                    (dataDigits[1] << 8) |
                    (dataDigits[0] << 12);

        /* Get the data of the record */
        for (i = 0; i < CC / 2; i++)
        {
            dataDigits[0] = getHex(g_recieveBuffer[4 * i + 9]);
            dataDigits[1] = getHex(g_recieveBuffer[4 * i + 10]);
            dataDigits[2] = getHex(g_recieveBuffer[4 * i + 11]);
            dataDigits[3] = getHex(g_recieveBuffer[4 * i + 12]);
            g_data[dataCounter] = (dataDigits[3] << 8) |
                                  (dataDigits[2] << 12) |
                                  (dataDigits[1] << 0) |
                                  (dataDigits[0] << 4);
            dataCounter++;
        }

        if (CC % 2 != 0)
        {
            dataDigits[0] = getHex(g_recieveBuffer[4 * (CC / 2) + 9]);
            dataDigits[1] = getHex(g_recieveBuffer[4 * (CC / 2) + 10]);
            g_data[dataCounter] = 0xFF00 | (dataDigits[0] << 4) | (dataDigits[1] << 0);
            HAL_FLASH_Unlock();
            for(int i=0;i<=CC/2+1;i++){
                HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,g_address+(i*4), (uint64_t *)g_data);
            }
              HAL_FLASH_Lock();
        }
        else
        {
        	HAL_FLASH_Unlock();
        	for(int i=0;i<=CC/2;i++){
           HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,g_address+(i*4), (uint64_t *)g_data);
        	}
        	 HAL_FLASH_Lock();
        }

        break;
    default:
        break;
    }
}

static void OTA_vidSetHighAddress(void)
{
    g_address = (getHex(g_recieveBuffer[9]) << 28) |
                (getHex(g_recieveBuffer[10]) << 24) |
                (getHex(g_recieveBuffer[11]) << 20) |
                (getHex(g_recieveBuffer[12]) << 16);
}


void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */

  /* USER CODE END USART1_IRQn 0 */
	if(g_recieveBuffer[0]==OTA_DATA_START_CHAR){
		HAL_SuspendTick();
	}
  /* USER CODE BEGIN USART1_IRQn 1 */
	HAL_UART_IRQHandler(&huart1);
  /* USER CODE END USART1_IRQn 1 */
}

static void OTA_vidCharReceived(uint8_t rec)
{
    g_recieveBuffer[g_recieveCounter] = rec;
    switch (rec)
    {
    case OTA_DATA_START_CHAR:
    case OTA_LINE_BREAK_CHAR:
        g_lineReceivedFlag = TRUE;
        break;
    case OTA_DATA_END_CHAR:
        g_finishReceiveFlag = TRUE;
        break;
    default:
        g_recieveCounter++;
        break;
    }
}


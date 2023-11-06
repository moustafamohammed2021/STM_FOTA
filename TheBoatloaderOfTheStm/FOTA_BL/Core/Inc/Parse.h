/*
 * Parse.h
 *
 *  Created on: Oct 22, 2023
 *      Author: MOUSTAFA
 */

#ifndef PARSE_H_
#define PARSE_H_

/********************************Includes************************************/
#include "usart.h"
#include "stm32f1xx_hal_uart.h"

extern  UART_HandleTypeDef huart1;
/*******************************Macros************************************/
#define OTA_RECIEVE_TIMEOUT_S   5

/* To change these values you Must change it too in NodeMCU code*/
#define OTA_DATA_START_CHAR     'X'
#define OTA_LINE_BREAK_CHAR     'Y'
#define OTA_DATA_END_CHAR       'Z'
#define OTA_READ_CONFIRM_CHAR   'R'



#define FALSE 0
#define TRUE  1

/********************************Function Like Macros************************************/

/********************************Data Types Decelerations************************************/

/********************************Soft Ware Interface Decelerations************************************/

void FOTA_vidInit(void);

void FOTA_vidRun(void);

#endif /* PARSE_H_ */

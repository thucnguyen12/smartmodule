/*
 * app_ota.h
 *
 *  Created on: Sep 19, 2022
 *      Author: thuc_nd
 */

#ifndef COMPONENTS_APP_UART_INCLUDE_APP_UART_H_
#define COMPONENTS_APP_UART_INCLUDE_APP_UART_H_



#include <stdint.h>
#include <stdbool.h>
#include "driver/uart.h"


//#define OTA_CONFIG_ADD_STR "http://192.168.2.241/pppos_client.bin"
#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)
#define USE_APP_CLI 1


void uart_event_task(void *pvParameters);



#endif /* COMPONENTS_APP_OTA_INCLUDE_APP_UART_H_ */

/*
 * app_ota.h
 *
 *  Created on: Sep 19, 2022
 *      Author: thuc_nd
 */

#ifndef COMPONENTS_APP_OTA_INCLUDE_APP_UART_H_
#define COMPONENTS_APP_OTA_INCLUDE_APP_UART_H_



#include <stdint.h>
#include <stdbool.h>

//#define OTA_CONFIG_ADD_STR "http://192.168.2.241/pppos_client.bin"
#define WEB_SERVER "http://192.168.2.241"
#define WEB_PORT "80"
#define WEB_PATH "/pppos_client.bin"

static void http_get_task(void *pvParameters);


#endif /* COMPONENTS_APP_OTA_INCLUDE_APP_UART_H_ */

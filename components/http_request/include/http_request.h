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
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "cJSON.h"
//#define OTA_CONFIG_ADD_STR "http://192.168.2.241/pppos_client.bin"
#define WEB_SERVER "btwork.bytechjsc.vn"
#define WEB_PORT "5500"
#define WEB_PATH "/api/server-info"
#define HTTP_SERVER "btwork.bytechjsc.vn/api/server-info"

typedef struct 
{
    char server_ip [32];
    int port;
    char username [32];
    char password [32];
} mqtt_info_struct ;


void http_get_task(void *pvParameters);

cJSON *parse_json(char *content);

#endif /* COMPONENTS_APP_OTA_INCLUDE_APP_UART_H_ */

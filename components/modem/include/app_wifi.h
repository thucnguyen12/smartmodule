/*
 * app_wifi.h
 *
 *  Created on: Apr 27, 2021
 *      Author: huybk
 */

#ifndef COMPONENTS_APP_WIFI_INCLUDE_APP_WIFI_H_
#define COMPONENTS_APP_WIFI_INCLUDE_APP_WIFI_H_

#define WIFI_MAXIMUM_RETRY           10
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_PING_SUCESS BIT2
#define WIFI_PING_TIMEOUT BIT3

#include <stdint.h>
#include <stdbool.h>



/**
 * @brief			Connect to AP
 */
void app_wifi_connect(char *ssid, char *password);

/**
 * @brief			Get ip status
 */
bool app_wifi_is_ip_acquired(void);

#endif /* COMPONENTS_APP_WIFI_INCLUDE_APP_WIFI_H_ */

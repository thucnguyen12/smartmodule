/*
 * app_ota.h
 *
 *  Created on: Sep 19, 2022
 *      Author: thuc_nd
 */

#ifndef COMPONENTS_APP_OTA_INCLUDE_APP_OTA_H_
#define COMPONENTS_APP_OTA_INCLUDE_APP_OTA_H_



#include <stdint.h>
#include <stdbool.h>

#define OTA_CONFIG_ADD_STR "http://192.168.2.241/pppos_client.bin"

char ota_url [128];


void advanced_ota_example_task(void *pvParameter);

#endif /* COMPONENTS_APP_OTA_INCLUDE_APP_OTA_H_ */

/*
 * app_ota.h
 *
 *  Created on: Sep 19, 2022
 *      Author: thuc_nd
 */

#ifndef COMPONENTS_APP_OTA_INCLUDE_APP_SNTP_H_
#define COMPONENTS_APP_OTA_INCLUDE_APP_SNTP_H_



#include <stdint.h>
#include <stdbool.h>

//#define OTA_CONFIG_ADD_STR "http://192.168.2.241/pppos_client.bin"
typedef struct
{
	uint8_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	uint8_t weekday;
} date_time_t;



uint32_t app_time(void);

uint32_t convert_date_time_to_second(date_time_t *t);

uint8_t get_weekday(date_time_t time);

void convert_second_to_date_time(uint32_t sec, date_time_t *t, uint8_t Calyear);

#endif /* COMPONENTS_APP_OTA_INCLUDE_APP_SNTP_H_ */

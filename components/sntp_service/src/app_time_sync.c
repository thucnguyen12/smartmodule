/*
 * app_wifi.c
 *
 *  Created on: Apr 27, 2021
 *      Author: huybk
 */

#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
//#include "esp_netif.h"
//#include "esp_system.h"
//#include "esp_wifi.h"
#include <string.h>
//RTC
#include "rtc.h"
#include "esp_sntp.h"
#include "app_time_sync.h"


static const char *TAG = "app_time_sync";

static const uint8_t day_in_month[12] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void time_sync_notification_cb(struct timeval *tv)
{

    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
#ifdef CONFIG_SNTP_TIME_SYNC_METHOD_SMOOTH
    sntp_set_sync_mode(SNTP_SYNC_MODE_SMOOTH);
#endif
    sntp_init();
}

static void obtain_time(void)
{
    /* this is not necessory cause we alreadey connected

    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK( esp_event_loop_create_default() );
    
    */

    /**
     * NTP server address could be aquired via DHCP,
     * see LWIP_DHCP_GET_NTP_SRV menuconfig option
     */

#ifdef LWIP_DHCP_GET_NTP_SRV
    sntp_servermode_dhcp(1);
#endif

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    // ESP_ERROR_CHECK(example_connect());
    
    initialize_sntp();

    // wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
    time(&now);
    localtime_r(&now, &timeinfo);

    //ESP_ERROR_CHECK( example_disconnect() );
}

uint32_t app_time(void)
{
    time_t now;
    static struct tm timeinfo = { 0 };
    static struct tm last_timeinfo = { 0 };
    date_time_t time_now_struct;
    if ((timeinfo.tm_hour - last_timeinfo.tm_hour) < 1)
    {
        return 0;
    }
    // need update time before get localtime
    time(&now);
    localtime_r(&now, &timeinfo);
    if (timeinfo.tm_year < (2016 - 1900)) {
        ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
        obtain_time();
        // update 'now' variable with current time
        time(&now);
    }
    setenv("TZ", "Etc/GMT+7", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    // Need convert timeinfo to date_time_t then send it to gd32;
    time_now_struct.year = timeinfo.tm_year;
    time_now_struct.month = timeinfo.tm_month;
    time_now_struct.day = timeinfo.tm_day;
    time_now_struct.hour = timeinfo.tm_hour;
    time_now_struct.minute = timeinfo.tm_minute;
    time_now_struct.second = timeinfo.tm_second;
    uint32_t timstamp = convert_date_time_to_second(&time_now_struct);

    char strftime_buf[64];
    strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
    ESP_LOGI(TAG, "The current date/time in New York is: %s", strftime_buf);
    return timstamp;
}


void convert_second_to_date_time(uint32_t sec, date_time_t *t, uint8_t Calyear)
{
	uint16_t day;
	uint8_t year;
	uint16_t days_of_year;
	uint8_t leap400;
	uint8_t month;

	t->second = sec % 60;
	sec /= 60;
	t->minute = sec % 60;
	sec /= 60;
	t->hour = sec % 24;

	if (Calyear == 0)
		return;

	day = (uint16_t)(sec / 24);

	year = FIRSTYEAR % 100;					   // 0..99
	leap400 = 4 - ((FIRSTYEAR - 1) / 100 & 3); // 4, 3, 2, 1

	for (;;)
	{
		days_of_year = 365;
		if ((year & 3) == 0)
		{
			days_of_year = 366; // leap year
			if (year == 0 || year == 100 || year == 200)
			{ // 100 year exception
				if (--leap400)
				{ // 400 year exception
					days_of_year = 365;
				}
			}
		}
		if (day < days_of_year)
		{
			break;
		}
		day -= days_of_year;
		year++; // 00..136 / 99..235
	}
	t->year = year + FIRSTYEAR / 100 * 100 - 2000; // + century
	if (days_of_year & 1 && day > 58)
	{		   // no leap year and after 28.2.
		day++; // skip 29.2.
	}

	for (month = 1; day >= day_in_month[month - 1]; month++)
	{
		day -= day_in_month[month - 1];
	}

	t->month = month; // 1..12
	t->day = day + 1; // 1..31
}

uint8_t get_weekday(date_time_t time)
{
	time.weekday = (time.day +=
					time.month < 3 ? time.year-- : time.year - 2,
					23 * time.month / 9 + time.day + 4 + time.year / 4 -
						time.year / 100 + time.year / 400);
	return time.weekday % 7;
}

uint32_t convert_date_time_to_second(date_time_t *t)
{
	uint8_t i;
	uint32_t result = 0;
	uint16_t idx, year;

	year = t->year + 2000;

	/* Calculate days of years before */
	result = (uint32_t)year * 365;
	if (t->year >= 1)
	{
		result += (year + 3) / 4;
		result -= (year - 1) / 100;
		result += (year - 1) / 400;
	}

	/* Start with 2000 a.d. */
	result -= 730485UL;

	/* Make month an array index */
	idx = t->month - 1;

	/* Loop thru each month, adding the days */
	for (i = 0; i < idx; i++)
	{
		result += day_in_month[i];
	}

	/* Leap year? adjust February */
	if (!(year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)))
	{
		if (t->month > 2)
		{
			result--;
		}
	}

	/* Add remaining days */
	result += t->day;

	/* Convert to seconds, add all the other stuff */
	result = (result - 1) * 86400L + (uint32_t)t->hour * 3600 +
			 (uint32_t)t->minute * 60 + t->second;
	return result;
}
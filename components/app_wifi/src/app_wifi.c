/*
 * app_wifi.c
 *
 *  Created on: Apr 27, 2021
 *      Author: huybk
 */

#include "app_wifi.h"
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include <string.h>
#include "nvs_flash.h"



static const char *TAG = "app_wifi";
static bool m_got_ip = false;
EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool init_sta = false;
// esp_netif_t *wifi_netif = NULL;
extern bool wifi_started;


static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        wifi_started = false;
        if (s_retry_num < WIFI_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGI(TAG, "connect to the AP fail");
        }
        
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        wifi_started = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_netif_t* esp_netif_create_wifi_sta(void)
{
    esp_netif_ip_info_t ip_info;
    esp_netif_inherent_config_t netif_wifi_config = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
    netif_wifi_config.route_prio = 1;
    netif_wifi_config.if_desc = "netif_wifi";
    esp_netif_config_t cfg = {
        .base = &netif_wifi_config,                 // use specific behaviour configuration
        .driver = NULL,
        .stack = ESP_NETIF_NETSTACK_DEFAULT_WIFI_STA, // use default WIFI-like network stack configuration
    };
    esp_netif_t *wifi_netif = esp_netif_new(&cfg);
    assert(wifi_netif);
    esp_netif_attach_wifi_station(wifi_netif);
    esp_wifi_set_default_wifi_sta_handlers();
    return wifi_netif;
}


void wifi_init_sta(char *ssid, char* password)
{
    s_wifi_event_group = xEventGroupCreate();

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    //ESP_ERROR_CHECK(esp_netif_init());
    
    // ESP_ERROR_CHECK(esp_event_loop_create_default());
    if (init_sta == false)
    {
        //esp_netif_create_default_wifi_sta();
        esp_netif_create_wifi_sta();
        init_sta = true;
    }
    
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_LOGI(TAG, "MARK HERE");
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,

            .pmf_cfg = {
                .capable = true,
                .required = false},
        },
    };

    memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
    memcpy(wifi_config.sta.password, password, strlen(password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished, ssid %s, password %s", ssid, password);

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by wifi_event_handler() (see above) */
/*    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
*/
    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */

/*    if (bits & WIFI_CONNECTED_BIT)
    {
        m_got_ip = true;
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 ssid, password);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 ssid, password);
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
*/
}

void app_wifi_connect(char *ssid, char *password)
{
    wifi_init_sta(ssid, password);
}

bool app_wifi_is_ip_acquired()
{
    return m_got_ip;
}
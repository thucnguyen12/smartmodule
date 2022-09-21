#include "app_mqtt.h"
#include <string.h>
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "mqtt_protocol";

void make_mqtt_topic_header(header_type_t header_type ,char *topic_hr, char *IMEI, char* str_out)
{
    // switch case
    
    switch (header_type)
    {
    case HEART_BEAT_HEADER:
        sprintf (str_out, "%s/d2g/heartbeat/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat heart beat header");
        break;
    case FIRE_ALARM_HEADER:
        sprintf (str_out, "%s/d2g/fire/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat fire header");
        break;
    case SENSOR_HEADER:
        sprintf (str_out, "%s/d2g/sensor/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat sensor header");
        break;
    case INFO_HEADER:
        sprintf (str_out, "%s/d2g/info/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat info header");
        break;
    case CONFIG_HEADER:
        sprintf (str_out, "%s/d2g/config/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat config header");
        break;
    case OTA_HEADER:
        sprintf (str_out, "%s/d2g/ota/%s", topic_hr, IMEI);
        ESP_LOGI(TAG, "creat ota header");
        break;
    default:
        break;
    }
}

void make_fire_alarm_protocol_payload (fire_status_t info, char * str_out)
{
    // chia thanh nhieu loai ban tin
    if (1)
    {
        uint32_t index = 0;
        index = sprintf (str_out + index, "{\r\n\"fireStatus\":%u\r\n", info.fire_status);
        index = sprintf (str_out + index, "{\r\n\"csq\":%u\r\n", info.csq);
        index = sprintf (str_out + index, "{\r\n\"connectedSenSor\":%u\r\n", info.sensor_cnt);
        index = sprintf (str_out + index, "{\r\n\"battery\":%u\r\n", info.battery);
        index = sprintf (str_out + index, "{\r\n\"InTestMode\":%s\r\n", info.inTestmode ? "1":"0");
        index = sprintf (str_out + index, "{\r\n\"temperature\":%s\r\n", info.inTestmode ? "1":"0");
//        if (info.networkInterface == 1 ) // ETH)
            index = sprintf (str_out + index, "{\r\n\"networkInterface\":%s\r\n", info.networkInterface);
//        else
//        {
//            index = sprintf (str_out + index, "{\r\n\"networkInterface\":wifi\r\n");
//        }
        index = sprintf (str_out + index, "{\r\n\"networkStatus eth\":%s\r\n",info.networkStatus.name.eth_internet);
        index = sprintf (str_out + index, "{\r\n\"networkStatus gsm\":%s\r\n",info.networkStatus.name.gsm_internet);
        index = sprintf (str_out + index, "{\r\n\"networkStatus wifi\":%s\r\n",info.networkStatus.name.wifi_internet);
        index = sprintf (str_out + index, "{\r\n\"fireZone\":%x\r\n",info.fire_zone);
    }   
}